from dataclasses import dataclass
from functools import lru_cache
import re
import struct


class ProfileError(ValueError):
    pass


def require(condition, message):
    if not condition:
        raise ProfileError(message)


def hex_value(value):
    return f"0x{value:X}"


@dataclass(frozen=True)
class Section:
    name: bytes
    virtual_size: int
    rva: int
    raw_size: int
    file_offset: int
    flags: int

    @property
    def executable(self):
        return bool(self.flags & 0x20000000)

    @property
    def writable(self):
        return bool(self.flags & 0x80000000)


@dataclass(frozen=True)
class GameProfile:
    rvas: dict[str, int]
    offsets: dict[str, int]
    menu_positions: list[int]
    prologues: dict[str, bytes]
    limiter: bytes
    refresh_jump: int


@lru_cache(maxsize=None)
def byte_pattern(pattern):
    tokens = pattern.split()
    require(tokens and any(token != "??" for token in tokens), "Firma de bytes vacia")
    expression = []
    for token in tokens:
        if token == "??":
            expression.append(b".")
        else:
            choices = [re.escape(bytes.fromhex(value)) for value in token.split("|")]
            expression.append(choices[0] if len(choices) == 1 else b"(?:" + b"|".join(choices) + b")")
    return re.compile(b"(?=" + b"".join(expression) + b")", re.DOTALL)


class GameImage:
    def __init__(self, data):
        self.data = data
        require(data[:2] == b"MZ" and len(data) >= 64, "El archivo no es un ejecutable PE")
        pe = struct.unpack_from("<I", data, 60)[0]
        require(data[pe:pe + 4] == b"PE\0\0" and pe + 24 <= len(data), "Cabecera PE incompleta")
        machine, count, self.timestamp = struct.unpack_from("<HHI", data, pe + 4)
        optional_size = struct.unpack_from("<H", data, pe + 20)[0]
        optional = pe + 24
        section_table = optional + optional_size
        require(optional_size >= 68 and section_table + 40 * count <= len(data), "Tabla de secciones PE incompleta")
        require(machine == 0x14C and struct.unpack_from("<H", data, optional)[0] == 0x10B,
                "El interpolador requiere un ejecutable x86 de 32 bits")
        self.base = struct.unpack_from("<I", data, optional + 28)[0]
        self.image_size, _, self.checksum = struct.unpack_from("<III", data, optional + 56)
        self.sections = []
        for index in range(count):
            record = struct.unpack_from("<8sIIII12xI", data, section_table + 40 * index)
            section = Section(*record)
            require(section.file_offset + section.raw_size <= len(data), "Seccion PE truncada")
            self.sections.append(section)

        version_pattern = rb"Binding of Isaac: Repentance\+ v((\d+(?:\.\d+){2,3})(?:\.[A-Za-z]\d+)?)\r?\n?\0"
        titles = set(re.findall(version_pattern, data))
        require(len(titles) == 1, "El ejecutable no identifica una unica build de Repentance+")
        version, release_version = titles.pop()
        self.version = version.decode("ascii")
        self.release_version = release_version.decode("ascii")

    def section_at(self, rva, size=1):
        return next((section for section in self.sections
                     if section.rva <= rva and rva + size <= section.rva + max(section.virtual_size, section.raw_size)), None)

    def read(self, rva, size):
        if size == 0:
            return b""
        section = self.section_at(rva, size)
        require(section is not None and rva - section.rva + size <= section.raw_size,
                f"Lectura fuera del archivo en {hex_value(rva)}")
        offset = section.file_offset + rva - section.rva
        return self.data[offset:offset + size]

    def integer(self, rva, size, signed=False):
        return int.from_bytes(self.read(rva, size), "little", signed=signed)

    def matches(self, pattern, within=None, *, all_sections=False):
        regex = byte_pattern(pattern)
        if within is not None:
            start, length = within
            section = self.section_at(start)
            require(section is not None and section.executable, f"Codigo fuera de seccion ejecutable en {hex_value(start)}")
            length = min(length, section.rva + section.raw_size - start)
            return [start + match.start() for match in regex.finditer(self.read(start, length))]
        return [section.rva + match.start() - section.file_offset
                for section in self.sections if section.raw_size and (all_sections or section.executable)
                for match in regex.finditer(self.data, section.file_offset, section.file_offset + section.raw_size)]

    def call_target(self, rva):
        require(self.read(rva, 1) == b"\xE8", f"Falta la llamada directa en {hex_value(rva)}")
        target = rva + 5 + self.integer(rva + 1, 4, signed=True)
        section = self.section_at(target)
        require(section is not None and section.executable, f"Llamada fuera del codigo en {hex_value(rva)}")
        return target


PROLOGUE_TARGETS = {
    "RenderPrologue": "RenderRva",
    "MenuRenderPrologue": "MenuRenderRva",
    "CharacterWheelPrologue": "CharacterWheelRenderRva",
    "StatsRenderPrologue": "StatsRenderRva",
    "OptionsRenderPrologue": "OptionsRenderRva",
    "PlayerChargeBarsRenderPrologue": "PlayerChargeBarsRenderRva",
    "PlayerBodyRenderPrologue": "PlayerBodyRenderRva",
    "PlayerIsHoldingItemPrologue": "PlayerIsHoldingItemRva",
    "FamiliarChargeBarRenderPrologue": "FamiliarChargeBarRenderRva",
}


def unique(name, candidates):
    examples = ", ".join(map(hex_value, candidates[:4]))
    require(len(candidates) == 1,
            f"{name}: {len(candidates)} coincidencias validas" + (f" ({examples})" if examples else ""))
    return candidates[0]


def resolve_anchor(image, name, rule, resolved):
    if "from_call" in rule:
        return image.call_target(resolved[rule["from_call"]])

    within = None
    if "within" in rule:
        owner, size = rule["within"]
        within = (resolved[owner], size)
    candidates = image.matches(rule["pattern"], within)
    call = rule.get("call")
    if call and "target" in call:
        target = resolved[call["target"]]
        candidates = [site for site in candidates
                      if image.call_target(site + call["at"]) == target]
    start = unique(name, candidates)
    if "read" not in rule:
        site = start + (call["at"] if call else 0)
        return image.call_target(site) if call and call.get("follow") else site

    offset, size = rule["read"]
    if rule.get("absolute"):
        rva = image.integer(start + offset, size) - image.base
        check_writable(image, name, rva)
        return rva
    member = image.integer(start + offset, size, signed=True)
    require(member >= 0, f"{name}: desplazamiento negativo")
    return member


def check_writable(image, name, rva):
    section = image.section_at(rva, 4)
    require(section is not None and section.writable and not section.executable,
            f"{name}: direccion de datos fuera de una seccion de escritura")


def validate_limiter(image, values, instructions):
    limiter_rva = values["FrameLimiterRva"]
    limiter = bytearray(image.read(limiter_rva, len(instructions.split())))
    require(byte_pattern(instructions).match(limiter), "FrameLimiterRva: instrucciones inesperadas")
    flag_rva = int.from_bytes(limiter[6:10], "little") - image.base
    check_writable(image, "SwapIntervalFlagRva", flag_rva)
    values["SwapIntervalFlagRva"] = flag_rva
    loop = limiter_rva + len(limiter) + int.from_bytes(limiter[-4:], "little", signed=True)
    require(loop == values["MainLoopRva"] < values["RenderCallRva"] < limiter_rva, "MainLoopRva: flujo del render inesperado")
    require(image.read(values["UpdateRva"], 8) == bytes.fromhex("55 8B EC 83 E4 F0 81 EC"),
            "UpdateRva: prologo incompatible con la llamada de Repentance+")
    limiter[6:10] = b"\0" * 4
    return bytes(limiter)


def menu_positions(image, rules, values, call_count=None):
    view = values["MenuViewOffset"]
    paper = values["MenuAnchorX"]
    require(values["MenuViewY"] == view + 4 and values["MenuAnchorY"] == paper + 4,
            "MenuRenderRva: las coordenadas del menu deben ser pares contiguos")
    positions = [view, paper]
    menu = (values["MenuRenderRva"], 0xA40)
    if call_count is not None:
        pattern = " ".join(["8D 8E ?? ?? ?? ?? E8 ?? ?? ?? ??"] * call_count)
        first = unique("MenuRenderRva/llamadas de paneles", image.matches(pattern, menu))
        panel_calls = [first + 11 * index for index in range(call_count)]
        for site in panel_calls:
            image.call_target(site + 6)

    for rule in rules:
        label = f"MenuPositionOffsets/{rule['panel']}"
        site = (panel_calls[rule["index"]] if call_count is not None
                else unique(label, image.matches(rule["caller"], menu)))
        require(image.read(site, 2) == b"\x8D\x8E", f"{label}: acceso al panel inesperado")
        panel = image.integer(site + 2, 4, signed=True)
        renderer = image.call_target(site + 6)
        if "member" in rule:
            require(panel == values[rule["member"]], f"{label}: el panel no coincide con {rule['member']}")
        if "renderer" in rule:
            require(renderer == values[rule["renderer"]], f"{label}: el renderizador no coincide")
        start = unique(label + "/dibujo", image.matches(rule["draw"], (renderer, 0x250)))
        x = image.integer(start + rule["x"][0], rule["x"][1], signed=True)
        y = image.integer(start + rule["y"][0], rule["y"][1], signed=True)
        require(panel >= 0 and x >= 0 and y == x + 4, f"{label}: coordenadas del panel inesperadas")
        positions.append(panel + x)
    require(len(positions) == len(set(positions)), "MenuPositionOffsets: hay posiciones duplicadas")
    return positions


def resolve_profile(image, signatures):
    values = {}
    for name, rule in signatures["anchors"].items():
        values[name] = resolve_anchor(image, name, rule, values)

    limiter = validate_limiter(image, values, signatures["limiter_pattern"])
    refresh = image.read(values["VSyncRefreshCheckRva"], 2)
    require(refresh[0] == 0x76, "VSyncRefreshCheckRva: salto condicional inesperado")
    if "update_wrapper_offset" in signatures:
        wrapper = image.call_target(values["RenderCallRva"] - 5)
        require(wrapper + signatures["update_wrapper_offset"] == values["UpdateCallRva"],
                "UpdateCallRva: la llamada de actualizacion no pertenece al bucle principal")
    require(values["OptionsPositionY"] == values["OptionsPositionOffset"] + 4,
            "OptionsPositionOffset: las coordenadas deben ser contiguas")
    require(values["SecretsScrollListOffset"] == values["SecretsScrollInOffset"] + 4 and
            values["SecretsSelectionOffset"] == values["SecretsScrollInOffset"] + 8,
            "SecretsScrollInOffset: disposicion del menu de secretos inesperada")
    positions = menu_positions(image, signatures["menu_panels"], values, signatures.get("menu_call_count"))

    patterns = signatures["runtime_patterns"]
    for name, pattern in patterns.items():
        found = unique(f"{name}/firma de la DLL", image.matches(pattern, all_sections=True))
        require(found == values[name], f"{name}: la firma de la DLL apunta a otra funcion")

    rvas = {name: value for name, value in values.items() if name.endswith("Rva")}
    require(len(rvas) == len(set(rvas.values())), "Dos direcciones del perfil apuntan al mismo lugar")
    offsets = {name: value for name, value in values.items() if name.endswith("Offset")}
    sizes = signatures["prologue_sizes"]
    prologues = {name: image.read(values[site], sizes[name])
                 for name, site in PROLOGUE_TARGETS.items() if site in rvas}
    return GameProfile(rvas, offsets, positions, prologues, limiter, refresh[1])


def render_header(image, profile, patterns, digest):
    lines = [
        "#pragma once",
        f"// Game version {image.version}",
        f"// Release version {image.release_version}",
        f"// Executable SHA-256 {digest}",
        "",
        "#include <array>",
        "#include <cstdint>",
        "",
    ]
    if "FamiliarChargeBarRenderRva" in profile.rvas:
        lines.extend(["#define GAME_HAS_FAMILIAR_CHARGE_BARS", ""])
    lines.extend([
        "namespace GameVersion {",
        f"inline constexpr uint32_t ExecutableTimestamp = {hex_value(image.timestamp)};",
        f"inline constexpr uint32_t ExecutableImageSize = {hex_value(image.image_size)};",
        f"inline constexpr uint32_t ExecutableChecksum = {hex_value(image.checksum)};",
        "inline constexpr int MainMenuManagerState = 1;",
        "inline constexpr int CharacterPaperLayerId = 3;",
        f"inline constexpr uint8_t VSyncRefreshCheckOffset = {hex_value(profile.refresh_jump)};",
    ])
    for name, value in (profile.rvas | profile.offsets).items():
        lines.append(f"inline constexpr uintptr_t {name} = {hex_value(value)};")
    position_values = ", ".join(map(hex_value, profile.menu_positions))
    lines.append(f"inline constexpr std::array<uintptr_t, {len(profile.menu_positions)}> MenuPositionOffsets = {{{position_values}}};")
    for name, payload in (profile.prologues | {"FrameLimiterBytes": profile.limiter}).items():
        encoded = ", ".join(f"0x{byte:02X}" for byte in payload)
        lines.append(f"inline constexpr std::array<uint8_t, {len(payload)}> {name} = {{{encoded}}};")
    lines.append(f'inline constexpr char RenderFramePattern[] = "{patterns["RenderFrameRva"]}";')
    lines.append(f'inline constexpr char AnimationStateRenderPattern[] = "{patterns["AnimationStateRenderRva"]}";')
    return "\n".join(lines + ["}", ""])
