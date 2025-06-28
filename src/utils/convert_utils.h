#ifndef CONVERT_UTILS_H
#define CONVERT_UTILS_H

#include "../tui/tui_app.h"
#include <string>

namespace ConvertUtils {

/**
 * @brief Converts a variable from one type to another.
 * @param sourceVar The source variable to convert.
 * @param targetTypeFlag A flag indicating the target type ("-m", "-m1", "-m2", "-v", "-f").
 * @return A new Variable object with the converted value.
 * @throws std::runtime_error if the conversion is not supported.
 */
Variable convertVariable(const Variable& sourceVar, const std::string& targetTypeFlag);

} // namespace ConvertUtils

#endif // CONVERT_UTILS_H
