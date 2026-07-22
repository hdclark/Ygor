from pathlib import Path

path = Path("src/YgorMeshesBooleanBounded/CanonicalGeometryAttachments.h")
text = path.read_text()
old = '#include "FloatingBits.h"\n'
new = '#include "FloatingBits.h"\n#include "PrecisionContext.h"\n#include "Sha256.h"\n'
if old in text and '#include "PrecisionContext.h"' not in text:
    path.write_text(text.replace(old, new, 1))
elif '#include "PrecisionContext.h"' not in text or '#include "Sha256.h"' not in text:
    raise SystemExit("canonical geometry precision includes not found")

path = Path("src/YgorMeshesBooleanBounded/CanonicalHalfedgeVerifier.h")
text = path.read_text()
attachment_line = (
    "      artifact.precision_attachment_digest_ !=\n"
    "          canonical_precision_attachment_digest(precision) ||\n"
)
if attachment_line not in text:
    needle = "      artifact.precision_digest_ != precision.digest() ||\n"
    if needle not in text:
        raise SystemExit("canonical verifier precision digest anchor not found")
    text = text.replace(needle, needle + attachment_line, 1)

basis_check = '''    if (triangle.basis.kind != predecessor.basis.kind ||\n        triangle.basis.operand != predecessor.basis.operand ||\n        triangle.basis.facet != predecessor.basis.facet ||\n        triangle.basis.ring != predecessor.basis.ring ||\n        triangle.basis.shell != predecessor.basis.shell ||\n        triangle.basis.dropped_axis != predecessor.basis.dropped_axis ||\n        triangle.basis.support_vertices != predecessor.basis.support_vertices ||\n        triangle.basis.predecessor_digest != predecessor.basis.predecessor_digest ||\n        triangle.basis.precision_digest != precision.digest() ||\n        triangle.basis.basis_digest != predecessor.basis.basis_digest)\n      return fail(canonical_halfedge_subcode::geometry_basis_mismatch);\n'''
if basis_check not in text:
    needle = "    const auto &predecessor = source.triangles()[triangle.source_triangle];\n"
    if needle not in text:
        raise SystemExit("canonical verifier triangle predecessor anchor not found")
    text = text.replace(needle, needle + basis_check, 1)
path.write_text(text)

determinism_path = Path(".github/component05_determinism_fix.py")
determinism = determinism_path.read_text()
marker = '\nreplace_once(\n    "src/YgorMeshesBooleanBounded/CanonicalHalfedgeVerifier.h",'
position = determinism.find(marker)
if position >= 0:
    determinism_path.write_text(
        determinism[:position] +
        '\n\nPath(".github/component05_determinism_fix.py").unlink()\n'
    )

Path(".github/component05_include_fix.py").unlink()
