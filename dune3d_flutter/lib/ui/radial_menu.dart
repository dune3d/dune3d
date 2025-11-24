import 'package:flutter/material.dart';
import 'dart:math' as math;

class RadialMenu extends StatefulWidget {
  final Offset position;
  final VoidCallback onClose;
  final Function(String) onItemSelected;

  const RadialMenu({
    super.key,
    required this.position,
    required this.onClose,
    required this.onItemSelected,
  });

  @override
  State<RadialMenu> createState() => _RadialMenuState();
}

class _RadialMenuState extends State<RadialMenu> with SingleTickerProviderStateMixin {
  late AnimationController _controller;
  late Animation<double> _scaleAnimation;

  final List<Map<String, dynamic>> _menuItems = [
    {'id': 'line', 'icon': Icons.edit, 'label': 'Line'},
    {'id': 'circle', 'icon': Icons.circle_outlined, 'label': 'Circle'},
    {'id': 'rect', 'icon': Icons.crop_square, 'label': 'Rect'},
    {'id': 'trim', 'icon': Icons.content_cut, 'label': 'Trim'},
    {'id': 'delete', 'icon': Icons.delete_outline, 'label': 'Delete'},
    {'id': 'select', 'icon': Icons.touch_app, 'label': 'Select'},
  ];

  @override
  void initState() {
    super.initState();
    _controller = AnimationController(
      vsync: this,
      duration: const Duration(milliseconds: 200),
    );
    _scaleAnimation = CurvedAnimation(parent: _controller, curve: Curves.easeOutBack);
    _controller.forward();
  }

  @override
  void dispose() {
    _controller.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    const double radius = 80.0;
    const double itemSize = 48.0;

    return Stack(
      children: [
        // Backdrop to close menu
        Positioned.fill(
          child: GestureDetector(
            onTap: widget.onClose,
            behavior: HitTestBehavior.translucent,
            child: Container(color: Colors.transparent),
          ),
        ),
        Positioned(
          left: widget.position.dx - radius - itemSize / 2,
          top: widget.position.dy - radius - itemSize / 2,
          child: ScaleTransition(
            scale: _scaleAnimation,
            child: SizedBox(
              width: (radius + itemSize) * 2,
              height: (radius + itemSize) * 2,
              child: Stack(
                alignment: Alignment.center,
                children: [
                  // Central Close/Back button
                  GestureDetector(
                    onTap: widget.onClose,
                    child: Container(
                      width: 40,
                      height: 40,
                      decoration: BoxDecoration(
                        color: Theme.of(context).colorScheme.primaryContainer,
                        shape: BoxShape.circle,
                        boxShadow: [
                          BoxShadow(
                            color: Colors.black.withOpacity(0.3),
                            blurRadius: 8,
                            offset: const Offset(0, 4),
                          )
                        ],
                      ),
                      child: Icon(Icons.close, size: 20, color: Theme.of(context).colorScheme.onPrimaryContainer),
                    ),
                  ),
                  // Radial items
                  ...List.generate(_menuItems.length, (index) {
                    final double angle = (2 * math.pi * index) / _menuItems.length - math.pi / 2;
                    final double x = radius * math.cos(angle);
                    final double y = radius * math.sin(angle);

                    return Transform.translate(
                      offset: Offset(x, y),
                      child: _RadialMenuItem(
                        icon: _menuItems[index]['icon'],
                        label: _menuItems[index]['label'],
                        onTap: () {
                          widget.onItemSelected(_menuItems[index]['id']);
                          widget.onClose();
                        },
                      ),
                    );
                  }),
                ],
              ),
            ),
          ),
        ),
      ],
    );
  }
}

class _RadialMenuItem extends StatelessWidget {
  final IconData icon;
  final String label;
  final VoidCallback onTap;

  const _RadialMenuItem({
    required this.icon,
    required this.label,
    required this.onTap,
  });

  @override
  Widget build(BuildContext context) {
    return GestureDetector(
      onTap: onTap,
      child: Column(
        mainAxisSize: MainAxisSize.min,
        children: [
          Container(
            width: 48,
            height: 48,
            decoration: BoxDecoration(
              color: Theme.of(context).colorScheme.surface,
              shape: BoxShape.circle,
              boxShadow: [
                BoxShadow(
                  color: Colors.black.withOpacity(0.2),
                  blurRadius: 4,
                  offset: const Offset(0, 2),
                )
              ],
            ),
            child: Icon(icon, color: Theme.of(context).colorScheme.onSurface),
          ),
          const SizedBox(height: 4),
          Text(
            label,
            style: const TextStyle(fontSize: 10, fontWeight: FontWeight.bold, color: Colors.white, shadows: [Shadow(color: Colors.black, blurRadius: 2)]),
          ),
        ],
      ),
    );
  }
}
