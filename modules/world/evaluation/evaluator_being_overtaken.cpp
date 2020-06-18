// Copyright (c) 2020 fortiss GmbH
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#include "modules/world/evaluation/evaluator_being_overtaken.hpp"

#include "modules/world/evaluation/labels/agent_near_label_function.hpp"
#include "modules/world/evaluation/labels/ego_accelerate_label_function.hpp"
#include "modules/world/evaluation/labels/right_of_label_function.hpp"

namespace modules {
namespace world {
namespace evaluation {

/// Do not accelerate while another vehicle is near and left (overtaking)!
const char EvaluatorBeingOvertaken::formula_[] =
    "G ((right#0 & near#0) -> !accel)";

const LabelFunctions EvaluatorBeingOvertaken::labels_ = {
    LabelFunctionPtr(new RightOfLabelFunction("right")),
    LabelFunctionPtr(new EgoAccelerateLabelFunction("accel", 0.5)),
    LabelFunctionPtr(new AgentNearLabelFunction("near", 3.0))};

const char EvaluatorBeingOvertakenAssumption::formula_[] =
    "G !(right#0 & near#0)";

const LabelFunctions EvaluatorBeingOvertakenAssumption::labels_ = {
    LabelFunctionPtr(new RightOfLabelFunction("right")),
    LabelFunctionPtr(new AgentNearLabelFunction("near", 3.0))};

}  // namespace evaluation
}  // namespace world
}  // namespace modules
