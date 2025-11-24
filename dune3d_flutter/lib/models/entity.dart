enum EntityType { line, circle, rect }

class SketchEntity {
  final String id;
  final EntityType type;
  // Simplified parameters for mock
  final Map<String, double> params;

  SketchEntity({required this.id, required this.type, required this.params});
}

class Sketch {
  final String id;
  final String name;
  final List<SketchEntity> entities;

  Sketch({required this.id, required this.name, this.entities = const []});
}
