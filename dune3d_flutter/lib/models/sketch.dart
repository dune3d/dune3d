import 'entity.dart';
import 'constraint.dart';

class SketchModel {
  final Sketch activeSketch;
  final List<Constraint> constraints;

  SketchModel({required this.activeSketch, this.constraints = const []});
}
