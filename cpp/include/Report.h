// Report.h — exports of a built project for the downstream tools:
// specification table (Excel), technical assignment text (ТЗ) and title block
// attributes for the AutoCAD dwg sheet set.
#pragma once

#include "Project.h"

#include <string>
#include <utility>
#include <vector>

namespace bsc {

using Attributes = std::vector<std::pair<std::string, std::string>>;

// ';'-separated UTF-8 CSV (with BOM) that opens directly in Excel.
std::string specificationCsv(const Project& project);

// Plain text technical assignment (ТЗ на проектирование).
std::string technicalAssignment(const Project& project);

// SITE_ID, OPERATOR, POSITION_TYPE, ... for the dwg title block / .scr script.
Attributes dwgAttributes(const Project& project);

// Human readable console report: configuration, conflicts and specification.
std::string formatReport(const Project& project);

}  // namespace bsc
