
import 'package:flutter/material.dart';
import 'core_bridge.dart';
import 'dart:ffi' as ffi;

class CoreTestPage extends StatefulWidget {
  const CoreTestPage({super.key});

  @override
  State<CoreTestPage> createState() => _CoreTestPageState();
}

class _CoreTestPageState extends State<CoreTestPage> {
  String _jsonOutput = 'Not initialized';
  ffi.Pointer<ffi.Void>? _doc;

  @override
  void initState() {
    super.initState();
    _initCore();
  }

  void _initCore() {
    try {
      final core = Dune3DCore();
      _doc = core.newDocument();
      final json = core.documentToJson(_doc!);
      setState(() {
        _jsonOutput = 'Document created. JSON:\n$json';
      });
    } catch (e) {
      setState(() {
        _jsonOutput = 'Error initializing core: $e';
      });
    }
  }

  void _solve() {
    if (_doc != null) {
      final core = Dune3DCore();
      final res = core.solveDocument(_doc!);
      setState(() {
        _jsonOutput += '\nSolve result: $res';
      });
    }
  }

  @override
  void dispose() {
    if (_doc != null) {
      Dune3DCore().deleteDocument(_doc!);
    }
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text('Core Integration Test')),
      body: Column(
        children: [
          ElevatedButton(onPressed: _solve, child: const Text('Solve')),
          Expanded(
            child: SingleChildScrollView(
              child: Padding(
                padding: const EdgeInsets.all(8.0),
                child: Text(_jsonOutput),
              ),
            ),
          ),
        ],
      ),
    );
  }
}
