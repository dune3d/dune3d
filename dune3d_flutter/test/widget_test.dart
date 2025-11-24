import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:dune3d_flutter/main.dart';
import 'package:dune3d_flutter/ui/editor_viewport.dart';
import 'package:dune3d_flutter/ui/radial_menu.dart';

void main() {
  testWidgets('App launches and shows main UI', (WidgetTester tester) async {
    // Build our app and trigger a frame.
    await tester.pumpWidget(const Dune3DApp());

    // Verify that the Viewport is present
    expect(find.byType(EditorViewport), findsOneWidget);

    // Verify that the Navigation Rail is present
    expect(find.byType(NavigationRail), findsOneWidget);

    // Verify FAB is present
    expect(find.byType(FloatingActionButton), findsOneWidget);
  });

  testWidgets('Radial menu opens on FAB tap', (WidgetTester tester) async {
    await tester.pumpWidget(const Dune3DApp());

    // Tap the add button.
    await tester.tap(find.byType(FloatingActionButton));
    await tester.pumpAndSettle();

    // Verify radial menu appears
    expect(find.byType(RadialMenu), findsOneWidget);

    // Tap outside to close
    // We need to tap somewhere safe. The radial menu covers the screen with a detector.
    // Let's tap top left
    await tester.tapAt(const Offset(10, 10));
    await tester.pumpAndSettle();

    expect(find.byType(RadialMenu), findsNothing);
  });

  testWidgets('Radial menu interactions', (WidgetTester tester) async {
     await tester.pumpWidget(const Dune3DApp());

     // Open menu
     await tester.tap(find.byType(FloatingActionButton));
     await tester.pumpAndSettle();

     // Find "Line" item
     final lineItem = find.text('Line');
     expect(lineItem, findsOneWidget);

     // Tap it
     await tester.tap(lineItem);
     await tester.pumpAndSettle();

     // Should close menu
     expect(find.byType(RadialMenu), findsNothing);
  });
}
