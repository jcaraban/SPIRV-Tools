// Copyright (c) 2017 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "eliminate_dead_functions_pass.h"

#include <unordered_set>

namespace spvtools {
namespace opt {

Pass::Status EliminateDeadFunctionsPass::Process(ir::Module* module) {
  InitializeProcessing(module);

  // Identify live functions first.  Those that are not live
  // are dead.
  std::unordered_set<const ir::Function*> live_function_set;
  ProcessFunction mark_live = [&live_function_set](ir::Function* fp) {
    live_function_set.insert(fp);
    return false;
  };
  ProcessReachableCallTree(mark_live, module);

  FindNamedOrDecoratedIds();
  bool modified = false;
  for (auto funcIter = module->begin(); funcIter != module->end();) {
    if (live_function_set.count(&*funcIter) == 0) {
      modified = true;
      EliminateFunction(&*funcIter);
      funcIter = funcIter.Erase();
    } else {
      ++funcIter;
    }
  }

  return modified ? Pass::Status::SuccessWithChange
                  : Pass::Status::SuccessWithoutChange;
}

void EliminateDeadFunctionsPass::EliminateFunction(ir::Function* func) {
  // Remove all of the instruction in the function body
  func->ForEachInst(
      [this](ir::Instruction* inst) {
        KillNamesAndDecorates(inst);
        get_def_use_mgr()->KillInst(inst);
      },
      true);
}
}  // namespace opt
}  // namespace spvtools
