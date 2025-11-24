
import 'dart:ffi' as ffi;
import 'dart:io';
import 'package:ffi/ffi.dart';

// Define C function signatures
typedef Dune3dInitFunc = ffi.Void Function();
typedef Dune3dInit = void Function();

typedef Dune3dNewDocumentFunc = ffi.Pointer<ffi.Void> Function();
typedef Dune3dNewDocument = ffi.Pointer<ffi.Void> Function();

typedef Dune3dDeleteDocumentFunc = ffi.Void Function(ffi.Pointer<ffi.Void>);
typedef Dune3dDeleteDocument = void Function(ffi.Pointer<ffi.Void>);

typedef Dune3dDocumentToJsonFunc = ffi.Pointer<ffi.Char> Function(ffi.Pointer<ffi.Void>);
typedef Dune3dDocumentToJson = ffi.Pointer<ffi.Char> Function(ffi.Pointer<ffi.Void>);

typedef Dune3dSolveDocumentFunc = ffi.Int Function(ffi.Pointer<ffi.Void>);
typedef Dune3dSolveDocument = int Function(ffi.Pointer<ffi.Void>);

typedef Dune3dFreeStringFunc = ffi.Void Function(ffi.Pointer<ffi.Char>);
typedef Dune3dFreeString = void Function(ffi.Pointer<ffi.Char>);

class Dune3DCore {
  static final Dune3DCore _instance = Dune3DCore._internal();
  late ffi.DynamicLibrary _lib;

  late Dune3dInit _init;
  late Dune3dNewDocument _newDocument;
  late Dune3dDeleteDocument _deleteDocument;
  late Dune3dDocumentToJson _documentToJson;
  late Dune3dSolveDocument _solveDocument;
  late Dune3dFreeString _freeString;

  factory Dune3DCore() {
    return _instance;
  }

  Dune3DCore._internal() {
    var libPath = 'libdune3d_core.so';
    if (Platform.isLinux) {
      // On Linux, we expect the library to be in the same directory as the executable or in LD_LIBRARY_PATH
      // For development, we might need to point to it explicitly if not installed.
      // Assuming it is copied to the build output directory or similar.
      // For now, we assume it's loadable by name.
      try {
        _lib = ffi.DynamicLibrary.open(libPath);
      } catch (e) {
        // Fallback for development environment if needed, or print error
        print('Failed to load $libPath: $e');
        // Try relative path for development
        _lib = ffi.DynamicLibrary.open('./libdune3d_core.so');
      }
    } else {
        throw UnsupportedError('Platform not supported');
    }

    _init = _lib.lookupFunction<Dune3dInitFunc, Dune3dInit>('dune3d_init');
    _newDocument = _lib.lookupFunction<Dune3dNewDocumentFunc, Dune3dNewDocument>('dune3d_new_document');
    _deleteDocument = _lib.lookupFunction<Dune3dDeleteDocumentFunc, Dune3dDeleteDocument>('dune3d_delete_document');
    _documentToJson = _lib.lookupFunction<Dune3dDocumentToJsonFunc, Dune3dDocumentToJson>('dune3d_document_to_json');
    _solveDocument = _lib.lookupFunction<Dune3dSolveDocumentFunc, Dune3dSolveDocument>('dune3d_solve_document');
    _freeString = _lib.lookupFunction<Dune3dFreeStringFunc, Dune3dFreeString>('dune3d_free_string');

    _init();
  }

  ffi.Pointer<ffi.Void> newDocument() {
    return _newDocument();
  }

  void deleteDocument(ffi.Pointer<ffi.Void> doc) {
    _deleteDocument(doc);
  }

  String documentToJson(ffi.Pointer<ffi.Void> doc) {
    final ptr = _documentToJson(doc);
    if (ptr == ffi.nullptr) return '';
    final str = ptr.cast<Utf8>().toDartString();
    _freeString(ptr);
    return str;
  }

  int solveDocument(ffi.Pointer<ffi.Void> doc) {
    return _solveDocument(doc);
  }
}
