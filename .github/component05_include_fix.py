from pathlib import Path

path = Path("src/YgorMeshesBooleanBounded/CanonicalGeometryAttachments.h")
text = path.read_text()
old = '#include "FloatingBits.h"\n'
new = '#include "FloatingBits.h"\n#include "PrecisionContext.h"\n#include "Sha256.h"\n'
if old in text and '#include "PrecisionContext.h"' not in text:
    path.write_text(text.replace(old, new, 1))
elif '#include "PrecisionContext.h"' not in text or '#include "Sha256.h"' not in text:
    raise SystemExit("canonical geometry precision includes not found")
Path(".github/component05_include_fix.py").unlink()
