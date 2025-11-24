import 'package:flutter/material.dart';

class EditorViewport extends StatefulWidget {
  const EditorViewport({super.key});

  @override
  State<EditorViewport> createState() => _EditorViewportState();
}

class _EditorViewportState extends State<EditorViewport> {
  double _scale = 1.0;
  Offset _offset = Offset.zero;

  @override
  Widget build(BuildContext context) {
    return GestureDetector(
      onScaleUpdate: (ScaleUpdateDetails details) {
        setState(() {
          _scale = details.scale;
          _offset += details.focalPointDelta;
        });
      },
      child: Container(
        color: const Color(0xFF222222),
        child: CustomPaint(
          painter: GridPainter(offset: _offset, scale: _scale),
          child: Container(),
        ),
      ),
    );
  }
}

class GridPainter extends CustomPainter {
  final Offset offset;
  final double scale;

  GridPainter({required this.offset, required this.scale});

  @override
  void paint(Canvas canvas, Size size) {
    final paint = Paint()
      ..color = Colors.grey.withOpacity(0.3)
      ..strokeWidth = 1.0;

    // Draw a simple grid
    double gridSize = 50.0 * scale;

    // Horizontal lines
    for (double i = 0; i < size.height; i += gridSize) {
      canvas.drawLine(Offset(0, i), Offset(size.width, i), paint);
    }

    // Vertical lines
    for (double i = 0; i < size.width; i += gridSize) {
      canvas.drawLine(Offset(i, 0), Offset(i, size.height), paint);
    }
  }

  @override
  bool shouldRepaint(GridPainter oldDelegate) => true;
}
