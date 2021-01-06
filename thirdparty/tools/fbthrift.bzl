#
# Copyright 2020 Weibo Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# @author ZhongXiu Hao <nmred.hao@gmail.com>

"""Build rule for generating C or C++ sources with fbthrift thrift1.
"""

def _fbthrift_cc_library_impl(ctx):
    output_path_attr = ctx.attr.name
    if ctx.attr.output_path:
      output_path_attr = ctx.attr.output_path

    outputs = [
      ctx.actions.declare_file("%s/gen-cpp2/%s_%s.h" % (output_path_attr, ctx.attr.name, "constants")),
      ctx.actions.declare_file("%s/gen-cpp2/%s_%s.cpp" % (output_path_attr, ctx.attr.name, "constants")),
      ctx.actions.declare_file("%s/gen-cpp2/%s_%s.h" % (output_path_attr, ctx.attr.name, "data")),
      ctx.actions.declare_file("%s/gen-cpp2/%s_%s.cpp" % (output_path_attr, ctx.attr.name, "data")),
      ctx.actions.declare_file("%s/gen-cpp2/%s_%s.h" % (output_path_attr, ctx.attr.name, "types")),
      ctx.actions.declare_file("%s/gen-cpp2/%s_%s.cpp" % (output_path_attr, ctx.attr.name, "types")),
      ctx.actions.declare_file("%s/gen-cpp2/%s_%s.tcc" % (output_path_attr, ctx.attr.name, "types")),
      ctx.actions.declare_file("%s/gen-cpp2/%s_%s.h" % (output_path_attr, ctx.attr.name, "metadata")),
      ctx.actions.declare_file("%s/gen-cpp2/%s_%s.h" % (output_path_attr, ctx.attr.name, "types_custom_protocol")),
    ]

    service_outs = []
    service_outs += [ctx.actions.declare_file("%s/gen-cpp2/%s.h" % (output_path_attr, f)) for f in ctx.attr.service_list]
    service_outs += [ctx.actions.declare_file("%s/gen-cpp2/%s.cpp" % (output_path_attr, f)) for f in ctx.attr.service_list]
    service_outs += [ctx.actions.declare_file("%s/gen-cpp2/%sAsyncClient.h" % (output_path_attr, f)) for f in ctx.attr.service_list]
    service_outs += [ctx.actions.declare_file("%s/gen-cpp2/%sAsyncClient.cpp" % (output_path_attr, f)) for f in ctx.attr.service_list]
    service_outs += [ctx.actions.declare_file("%s/gen-cpp2/%s_custom_protocol.h" % (output_path_attr, f)) for f in ctx.attr.service_list]
    service_outs += [ctx.actions.declare_file("%s/gen-cpp2/%s_processmap_binary.cpp" % (output_path_attr, f)) for f in ctx.attr.service_list]
    service_outs += [ctx.actions.declare_file("%s/gen-cpp2/%s_processmap_compact.cpp" % (output_path_attr, f)) for f in ctx.attr.service_list]
    service_outs += [ctx.actions.declare_file("%s/gen-cpp2/%s.tcc" % (output_path_attr, f)) for f in ctx.attr.service_list]

    output_path = outputs[0].dirname.replace("gen-cpp2", "")

    # Argument list
    args = ctx.actions.args()
    args.add("--gen", "mstch_cpp2:%s" % ctx.attr.options)
    args.add("-o", output_path)
    args.add("-I", "`dirname " + ctx.file.src.dirname + "`")
    args.add("-I", "`dirname " + ctx.build_file_path + "`")
    args.add("-I", ".")
    args.add("-v")
    args.add(ctx.file.src.path)

    output_list = outputs + service_outs 

    inputs = ctx.files.thrift_include_files + ctx.files.src
    ctx.actions.run(
        executable = ctx.executable._thrift1,
        arguments = [args],
        inputs = inputs,
        outputs = output_list,
        mnemonic = "fbthrift",
        progress_message = "Generating %s from %s" %
                           (
                              output_path,
                              ctx.file.src.short_path,
                           ),
    )

    return [DefaultInfo(files = depset(direct = outputs + service_outs))]

gen_fbthrift = rule(
    implementation = _fbthrift_cc_library_impl,
    doc = "Generate C/C++-language sources from a Yacc file using fbthrift.",
    attrs = {
        "src": attr.label(
            mandatory = True,
            allow_single_file = [".thrift"],
            doc = "The .thrift source file for this rule",
        ),
        "thrift_include_files": attr.label_list(
            allow_files = [".thrift"],
            doc = "The .thrift source file for this rule",
        ),
        "options": attr.string(
            doc = "A list of options to pass to thrift1.  These are " +
                  "subject to $(location ...) expansion.",
        ),
        "output_path": attr.string(
            doc = "A list of options to pass to thrift1.  These are " +
                  "subject to $(location ...) expansion.",
        ),
        "service_list": attr.string_list(
            doc = "A list of options to pass to thrift1.  These are " +
                  "subject to $(location ...) expansion.",
        ),
        "_thrift1": attr.label(
            default = Label("@fbthrift//:thrift1"),
            executable = True,
            cfg = "host",
        ),
    },
    output_to_genfiles = True,
)
