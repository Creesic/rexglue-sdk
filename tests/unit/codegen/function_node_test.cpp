/**
 * @file        tests/unit/codegen/function_node_test.cpp
 * @brief       Unit tests for FunctionNode code emission
 *
 * @copyright   Copyright (c) 2026 Tom Clay <tomc@tctechstuff.com>
 * @license     BSD 3-Clause License
 */

#include <catch2/catch_test_macros.hpp>

#include <rex/codegen/binary_view.h>
#include <rex/codegen/config.h>
#include <rex/codegen/function_graph.h>

TEST_CASE("FunctionNode: empty stub emits named prologue for guest tracing", "[codegen]") {
  rex::codegen::BinaryView binary;
  rex::codegen::RecompilerConfig config;
  rex::codegen::FunctionGraph graph;
  rex::codegen::EmitContext ctx{
      .binary = binary,
      .config = config,
      .graph = graph,
      .entryPoint = 0,
      .resolver = nullptr,
  };
  rex::codegen::FunctionNode fn(0x827CBFC0, 0x10, rex::codegen::FunctionAuthority::CONFIG);

  std::string cpp = fn.emitCpp(ctx);

  CHECK(cpp.find("DEFINE_REX_FUNC(sub_827CBFC0)") != std::string::npos);
  CHECK(cpp.find("REX_FUNC_PROLOGUE(sub_827CBFC0);") != std::string::npos);
  CHECK(cpp.find("REX_FUNC_PROLOGUE();") == std::string::npos);
}
