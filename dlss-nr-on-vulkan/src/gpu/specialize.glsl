/* ID 0 fixes the operation, operand widths and publish at pipeline creation.
 * UINT_MAX preserves the original push-constant path for comparison. Include
 * after the push-constant block so the fallback can read pc.flags. */
layout(constant_id = 0) const uint SPECIALIZED_FLAGS = 0xFFFFFFFFu;
uint operation_flags() {
    return SPECIALIZED_FLAGS == 0xFFFFFFFFu ? pc.flags : SPECIALIZED_FLAGS;
}
