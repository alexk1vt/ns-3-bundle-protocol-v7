# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

# def options(opt):
#     pass

# def configure(conf):
#     conf.check_nonfatal(header_name='stdint.h', define_name='HAVE_STDINT_H')

def build(bld):
    module = bld.create_ns3_module('bundle-protocol', ['core', 'network','internet', 'ltp-protocol'])
    module.source = [
        'model/bp-cla-protocol.cc',
        'model/bp-tcp-cla-protocol.cc',
        'model/bp-ltp-cla-protocol.cc',
        'model/bp-endpoint-id.cc',
        'model/bp-header.cc',
        'model/bp-payload-header.cc',
        'model/bundle-protocol.cc',
        'model/bp-routing-protocol.cc',
        'model/bp-static-routing-protocol.cc',
        'model/bp-sdnv.cc',
        'model/bp-bundle.cc',
        'model/bp-canonical-block.cc',
        'model/bp-primary-block.cc',
        'helper/bundle-protocol-helper.cc',
        'helper/bundle-protocol-container.cc'
        ]
    #module.lib=['ltp-protocol']
    module_test = bld.create_ns3_module_test_library('bundle-protocol')
    module_test.source = [
        'test/bundle-protocol-test-suite.cc',
        ]

    headers = bld(features='ns3header')
    headers.module = 'bundle-protocol'
    headers.source = [
        'model/bp-cla-protocol.h',
        'model/bp-tcp-cla-protocol.h',
        'model/bp-ltp-cla-protocol.h',
        'model/bp-endpoint-id.h',
        'model/bp-header.h',
        'model/bp-payload-header.h',
        'model/bundle-protocol.h',
        'model/bp-routing-protocol.h',
        'model/bp-static-routing-protocol.h',
        'model/bp-sdnv.h',
        'model/bp-bundle.h',
        'model/bp-canonical-block.h',
        'model/bp-primary-block.h',
        'model/json.hpp',
        'helper/bundle-protocol-helper.h',
        'helper/bundle-protocol-container.h'
        ]

    if bld.env.ENABLE_EXAMPLES:
        bld.recurse('examples')

    # bld.ns3_python_bindings()

