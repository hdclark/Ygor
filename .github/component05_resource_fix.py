from pathlib import Path

for name in (
    ".github/component05_include_fix.py",
    ".github/component05_determinism_fix.py",
):
    fix_path = Path(name)
    if fix_path.exists():
        exec(compile(fix_path.read_text(), str(fix_path), "exec"))

path = Path("src/YgorMeshesBooleanBounded/CanonicalHalfedgeBuildCore.h")
text = path.read_text()
replacements = {
    "!checked_multiply(entities, std::uint64_t{512}, extra)":
        "!checked_multiply(entities, std::uint64_t{4096}, extra)",
    "!checked_multiply(validated_->facets().size(), std::uint64_t{2048}, extra)":
        "!checked_multiply(validated_->facets().size(), std::uint64_t{8192}, extra)",
    "!checked_multiply(validated_->shells().size(), std::uint64_t{2048}, extra)":
        "!checked_multiply(validated_->shells().size(), std::uint64_t{8192}, extra)",
    "!checked_multiply(source_->canonical_bytes().size(), std::uint64_t{8}, extra)":
        "!checked_multiply(source_->canonical_bytes().size(), std::uint64_t{16}, extra)",
}
for old, new in replacements.items():
    if old in text:
        text = text.replace(old, new, 1)
    elif new not in text:
        raise SystemExit(f"resource estimate form not found: {old}")
path.write_text(text)
