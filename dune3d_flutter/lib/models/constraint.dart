class Constraint {
  final String id;
  final String type; // e.g., 'coincident', 'horizontal', 'distance'
  final List<String> entityIds;
  final double? value;

  Constraint({required this.id, required this.type, required this.entityIds, this.value});
}
