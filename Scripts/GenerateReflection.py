"""
Generate a small reflection registration unit from UPROPERTY annotations.

Supported declaration shape:

    UPROPERTY(EditAnywhere, DisplayName="Blend Alpha", Min=0.0, Max=1.0, Speed=0.01, Animatable)
    float BlendAlpha = 0.0f;

    UPROPERTY(VisibleAnywhere)
    UAnimSequence* Anim = nullptr;

Only EditAnywhere and VisibleAnywhere are accepted as access specifiers.
"""

from __future__ import annotations

import re
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
PROJECT_DIR = ROOT / "JSEngine"
SOURCE_DIR = PROJECT_DIR / "Source"
OUTPUT_DIR = SOURCE_DIR / "Generated"
OUTPUT_CPP = OUTPUT_DIR / "Reflection.generated.cpp"


DECLARE_CLASS_RE = re.compile(r"DECLARE_CLASS\s*\(\s*([A-Za-z_]\w*)\s*,\s*([A-Za-z_]\w*)\s*\)")
DECL_RE = re.compile(
    r"^(?P<type>[A-Za-z_]\w*(?:::[A-Za-z_]\w*)?(?:\s*[*&])?)\s+"
    r"(?P<name>[A-Za-z_]\w*)\s*(?:[={].*)?;\s*$"
)


TYPE_MAP = {
    "bool": "EPropertyType::Bool",
    "int": "EPropertyType::Int",
    "int32": "EPropertyType::Int",
    "float": "EPropertyType::Float",
    "FVector": "EPropertyType::Vec3",
    "FVector4": "EPropertyType::Vec4",
    "FColor": "EPropertyType::Color",
    "FString": "EPropertyType::String",
    "FName": "EPropertyType::Name",
}


def strip_line_comment(line: str) -> str:
    return line.split("//", 1)[0].strip()


def normalize_type(type_text: str) -> str:
    return re.sub(r"\s+", "", type_text.strip())


def get_property_type(type_text: str) -> tuple[str, str]:
    normalized = normalize_type(type_text)
    if normalized.endswith("*"):
        object_type = normalized[:-1]
        return "EPropertyType::ObjectRef", f"&{object_type}::s_TypeInfo"

    if normalized in TYPE_MAP:
        return TYPE_MAP[normalized], "nullptr"

    raise ValueError(f"Unsupported UPROPERTY type: {type_text}")


def split_metadata_args(args_text: str) -> list[str]:
    args: list[str] = []
    current: list[str] = []
    in_string = False
    escaped = False

    for char in args_text:
        if escaped:
            current.append(char)
            escaped = False
            continue

        if char == "\\" and in_string:
            current.append(char)
            escaped = True
            continue

        if char == '"':
            in_string = not in_string
            current.append(char)
            continue

        if char == "," and not in_string:
            arg = "".join(current).strip()
            if arg:
                args.append(arg)
            current = []
            continue

        current.append(char)

    arg = "".join(current).strip()
    if arg:
        args.append(arg)

    return args


def find_uproperty_args(line: str) -> str | None:
    marker = "UPROPERTY"
    start = line.find(marker)
    if start < 0:
        return None

    open_index = line.find("(", start + len(marker))
    if open_index < 0:
        return None

    depth = 0
    in_string = False
    escaped = False
    for index in range(open_index, len(line)):
        char = line[index]
        if escaped:
            escaped = False
            continue
        if char == "\\" and in_string:
            escaped = True
            continue
        if char == '"':
            in_string = not in_string
            continue
        if in_string:
            continue
        if char == "(":
            depth += 1
        elif char == ")":
            depth -= 1
            if depth == 0:
                return line[open_index + 1:index]

    return None


def parse_metadata(args_text: str, path: Path, line_number: int) -> dict[str, str]:
    args = split_metadata_args(args_text)
    if not args:
        raise ValueError(f"{path}:{line_number}: UPROPERTY requires EditAnywhere or VisibleAnywhere")

    access = args[0]
    if access not in {"EditAnywhere", "VisibleAnywhere"}:
        raise ValueError(
            f"{path}:{line_number}: only UPROPERTY(EditAnywhere) and UPROPERTY(VisibleAnywhere) are supported"
        )

    metadata = {
        "access": access,
        "display_name": "nullptr",
        "min": "0.0f",
        "max": "0.0f",
        "speed": "0.1f",
        "usage_flags": "EPropertyUsageFlags::None",
        "type_override": "",
    }

    usage_flags: list[str] = []
    for arg in args[1:]:
        if arg == "Animatable":
            usage_flags.append("EPropertyUsageFlags::Animatable")
            continue

        if "=" not in arg:
            raise ValueError(f"{path}:{line_number}: unsupported UPROPERTY metadata '{arg}'")

        key, value = [part.strip() for part in arg.split("=", 1)]
        if key == "DisplayName":
            if not (value.startswith('"') and value.endswith('"')):
                raise ValueError(f"{path}:{line_number}: DisplayName must be a quoted string")
            metadata["display_name"] = value
        elif key == "Min":
            metadata["min"] = value
        elif key == "Max":
            metadata["max"] = value
        elif key == "Speed":
            metadata["speed"] = value
        elif key == "Type":
            if value != "SceneComponentRef":
                raise ValueError(f"{path}:{line_number}: unsupported Type override '{value}'")
            metadata["type_override"] = "EPropertyType::SceneComponentRef"
        else:
            raise ValueError(f"{path}:{line_number}: unsupported UPROPERTY metadata key '{key}'")

    if usage_flags:
        metadata["usage_flags"] = " | ".join(usage_flags)

    return metadata


def make_float_literal(value: str) -> str:
    value = value.strip()
    if value.endswith(("f", "F")):
        return value

    if re.fullmatch(r"[+-]?(?:\d+(?:\.\d*)?|\.\d+)", value):
        return f"{value}f"

    return value


def find_class_ranges(text: str) -> list[tuple[str, int, int]]:
    ranges: list[tuple[str, int, int]] = []
    for match in DECLARE_CLASS_RE.finditer(text):
        class_name = match.group(1)
        brace_start = text.rfind("{", 0, match.start())
        if brace_start < 0:
            continue

        depth = 0
        end = -1
        for index in range(brace_start, len(text)):
            char = text[index]
            if char == "{":
                depth += 1
            elif char == "}":
                depth -= 1
                if depth == 0:
                    end = index
                    break

        if end >= 0:
            ranges.append((class_name, brace_start, end))

    return ranges


def parse_header(path: Path) -> dict[str, list[dict[str, str]]]:
    text = path.read_text(encoding="utf-8-sig", errors="ignore")
    lines = text.splitlines()
    line_offsets: list[int] = []
    cursor = 0
    for line in lines:
        line_offsets.append(cursor)
        cursor += len(line) + 1

    class_ranges = find_class_ranges(text)
    if not class_ranges:
        return {}

    properties_by_class: dict[str, list[dict[str, str]]] = {}
    pending_metadata: dict[str, str] | None = None
    pending_line = 0

    for line_index, line in enumerate(lines):
        offset = line_offsets[line_index]
        active_class = None
        for class_name, start, end in class_ranges:
            if start <= offset <= end:
                active_class = class_name
                break

        clean = strip_line_comment(line)
        if not active_class:
            pending_metadata = None
            continue

        property_args = find_uproperty_args(clean)
        if property_args is not None:
            pending_metadata = parse_metadata(property_args, path, line_index + 1)
            pending_line = line_index + 1
            continue

        if pending_metadata is None:
            continue

        if not clean or clean in {"public:", "protected:", "private:"}:
            continue

        declaration = DECL_RE.match(clean)
        if not declaration:
            raise ValueError(
                f"{path}:{pending_line}: UPROPERTY must be followed by a supported single-line member declaration"
            )

        type_text = declaration.group("type")
        member_name = declaration.group("name")
        property_type, object_type = get_property_type(type_text)
        if pending_metadata["type_override"]:
            property_type = pending_metadata["type_override"]
            object_type = "nullptr"

        properties_by_class.setdefault(active_class, []).append(
            {
                "access": pending_metadata["access"],
                "name": member_name,
                "property_type": property_type,
                "object_type": object_type,
                "display_name": pending_metadata["display_name"],
                "min": pending_metadata["min"],
                "max": pending_metadata["max"],
                "speed": pending_metadata["speed"],
                "usage_flags": pending_metadata["usage_flags"],
            }
        )
        pending_metadata = None

    return properties_by_class


def main() -> None:
    reflected: dict[str, dict[str, object]] = {}

    for header in sorted(SOURCE_DIR.rglob("*.h")):
        parsed = parse_header(header)
        for class_name, properties in parsed.items():
            if not properties:
                continue
            rel_header = header.relative_to(SOURCE_DIR).as_posix()
            reflected[class_name] = {
                "header": rel_header,
                "properties": properties,
            }

    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

    lines: list[str] = [
        "// Auto-generated by Scripts/GenerateReflection.py. Do not edit manually.",
        "",
        "#include <cstddef>",
        "#include \"Object/Reflection.h\"",
        "",
    ]

    for data in reflected.values():
        lines.append(f"#include \"{data['header']}\"")

    lines.append("")
    for class_name in reflected:
        lines.append(f"void RegisterGeneratedReflection_{class_name}();")

    lines.extend(["", "namespace", "{"])
    for class_name in reflected:
        lines.append(f"    struct FAutoRegister_{class_name}")
        lines.append("    {")
        lines.append(f"        FAutoRegister_{class_name}()")
        lines.append("        {")
        lines.append(f"            RegisterGeneratedReflection_{class_name}();")
        lines.append("        }")
        lines.append("    };")
        lines.append("")
        lines.append(f"    FAutoRegister_{class_name} GAutoRegister_{class_name};")
        lines.append("")
    lines.append("}")
    lines.append("")

    for class_name, data in reflected.items():
        properties = data["properties"]
        lines.append(f"void RegisterGeneratedReflection_{class_name}()")
        lines.append("{")
        lines.append(f"    static const FPropertyMeta Properties[] =")
        lines.append("    {")
        for prop in properties:  # type: ignore[assignment]
            lines.append(
                f"        {{ \"{prop['name']}\", {prop['display_name']}, {prop['property_type']}, "
                f"offsetof({class_name}, {prop['name']}), "
                f"EPropertyAccess::{prop['access']}, {prop['object_type']}, "
                f"{make_float_literal(prop['min'])}, "
                f"{make_float_literal(prop['max'])}, "
                f"{make_float_literal(prop['speed'])}, "
                f"{prop['usage_flags']} }},"
            )
        lines.append("    };")
        lines.append("")
        lines.append(
            f"    FReflectionRegistry::Get().RegisterProperties("
            f"&{class_name}::s_TypeInfo, Properties, "
            f"static_cast<uint32>(sizeof(Properties) / sizeof(Properties[0])));"
        )
        lines.append("}")
        lines.append("")

    OUTPUT_CPP.write_text("\n".join(lines), encoding="utf-8", newline="\r\n")
    print(f"Generated {OUTPUT_CPP.relative_to(ROOT)} with {len(reflected)} reflected classes.")


if __name__ == "__main__":
    main()
