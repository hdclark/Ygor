#include "YgorMeshesBooleanProductContract.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <set>
#include <stdexcept>
#include <tuple>

namespace ygor {
namespace mesh_boolean {

#include "YgorMeshesBooleanProductContractInternal.inc"
#include "YgorMeshesBooleanProductContractOptions.inc"
#include "YgorMeshesBooleanProductContractBackend.inc"
#include "YgorMeshesBooleanProductContractReplay.inc"

} // namespace mesh_boolean
} // namespace ygor
