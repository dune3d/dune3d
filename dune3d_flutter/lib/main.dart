import 'package:flutter/material.dart';
import 'ui/widgets.dart';

void main() {
  runApp(const Dune3DApp());
}

class Dune3DApp extends StatelessWidget {
  const Dune3DApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'Dune 3D',
      theme: ThemeData(
        useMaterial3: true,
        brightness: Brightness.dark,
        colorSchemeSeed: Colors.blueGrey,
        scaffoldBackgroundColor: const Color(0xFF1E1E1E),
      ),
      home: const MainScreen(),
    );
  }
}

class MainScreen extends StatefulWidget {
  const MainScreen({super.key});

  @override
  State<MainScreen> createState() => _MainScreenState();
}

class _MainScreenState extends State<MainScreen> {
  // Navigation rail / toolbar state
  int _selectedIndex = 0;
  Offset? _radialMenuPosition;

  void _showRadialMenu(Offset position) {
    setState(() {
      _radialMenuPosition = position;
    });
  }

  void _hideRadialMenu() {
    setState(() {
      _radialMenuPosition = null;
    });
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      body: Stack(
        children: [
          Row(
            children: [
          // Side Toolbar (Left) - Optimized for thumb use
          NavigationRail(
            selectedIndex: _selectedIndex,
            onDestinationSelected: (int index) {
              setState(() {
                _selectedIndex = index;
              });
            },
            labelType: NavigationRailLabelType.all,
            destinations: const <NavigationRailDestination>[
               NavigationRailDestination(
                icon: Icon(Icons.mode_edit_outline),
                selectedIcon: Icon(Icons.mode_edit),
                label: Text('Sketch'),
              ),
              NavigationRailDestination(
                icon: Icon(Icons.transform),
                selectedIcon: Icon(Icons.transform),
                label: Text('Constraints'),
              ),
              NavigationRailDestination(
                icon: Icon(Icons.view_in_ar),
                selectedIcon: Icon(Icons.view_in_ar_outlined),
                label: Text('3D View'),
              ),
            ],
          ),
          const VerticalDivider(thickness: 1, width: 1),
          // Main Content Area
          Expanded(
            child: Stack(
              children: [
                // The Viewport (3D or 2D Sketch)
                Positioned.fill(
                  child: GestureDetector(
                    onLongPressStart: (details) {
                      _showRadialMenu(details.globalPosition);
                    },
                    child: const EditorViewport(),
                  ),
                ),

                // Floating Action Buttons / Quick Tools
                Positioned(
                  bottom: 20,
                  right: 20,
                  child: FloatingActionButton(
                    onPressed: () {
                      // Example center position for FAB trigger
                       _showRadialMenu(MediaQuery.of(context).size.center(Offset.zero));
                    },
                    child: const Icon(Icons.add),
                  ),
                ),
              ],
            ),
          ),
        ],
      ),
      if (_radialMenuPosition != null)
        RadialMenu(
          position: _radialMenuPosition!,
          onClose: _hideRadialMenu,
          onItemSelected: (toolId) {
            print('Selected tool: $toolId');
            // TODO: Handle tool selection
          },
        ),
      ], // Stack children
      ), // Stack
    );
  }
}
