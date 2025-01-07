#include "ConditionTester.h"

bool ConditionTester::isStepVisible(const InstallStep& step) const {
  const auto fileDependencies = step.visible.dependencies.fileDependencies;
  const auto flagDependencies = step.visible.dependencies.flagDependencies;
  const auto globalOperatorType = step.visible.dependencies.operatorType;

  if (fileDependencies.empty() && flagDependencies.empty()) {
    return true;
  }

  // For the globalOperatorType
  // Evaluate all conditions and store the results in a vector<bool>, then return based on operator.
  // These aren't expensive to calculate so rather than do some fancy logic to short-circuit, just calculate all of 'em.
  std::vector<bool> results;
  for (auto fileDependency : fileDependencies) {

  }
  for (const auto& flagDependency : flagDependencies) {
    results.emplace_back(mStateManager->getFlag(flagDependency.flag) == flagDependency.value);
  }

  if (globalOperatorType == OperatorTypeEnum::AND) {
    return std::ranges::all_of(results, [](const bool result) { return result; });
  }
  if (globalOperatorType == OperatorTypeEnum::OR) {
    return std::ranges::any_of(results, [](const bool result) { return result; });
  }
  // Not sure why this would happen, but it's here for now.
  return true;
}

