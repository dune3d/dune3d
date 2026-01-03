# Version 1.3.90

## New Features

 - add DXF export ([3cc35b8](https://github.com/dune3d/dune3d/commit/3cc35b864d89d25d3ee2758344dd9f7e340bf828))
 - support multiple source groups for array and mirror groups ([81dcdd4](https://github.com/dune3d/dune3d/commit/81dcdd47adeae7d99d7b3a73a4ce43578890359f))
 - bezier/bezier, bezier/arc same curvature constraint ([ec69edc](https://github.com/dune3d/dune3d/commit/ec69edc5cdf412b3204b0e86982ef38cdc2385e2), [be266a4](https://github.com/dune3d/dune3d/commit/be266a4921feba7fa6348a3c580258ab2d3918c0))
 - clickable cube to replace lollipop axes ([cde3c47](https://github.com/dune3d/dune3d/commit/cde3c47723b2db7a2cd4cd2137dad74e1068263f))
 - action for exporting all bodies as STEP model ([3f80b48](https://github.com/dune3d/dune3d/commit/3f80b489f984ffcdd1c93f321af9ff1cf592f2e3))
 - tool for converting between point on line and midpoint constraints ([568a62d](https://github.com/dune3d/dune3d/commit/568a62d3c0879c05c9ce020209e83e29161fe6a2))
 - view option to hide irrelevant workplanes ([b8a24a6](https://github.com/dune3d/dune3d/commit/b8a24a6274ce3b0f9665fd69b880d7b3c017141d))

## Enhancements

 - move constraints after creating them ([5bf841a](https://github.com/dune3d/dune3d/commit/5bf841a2cff23c3d8e116e5bd5610969c6dc4abd))
 - make rendering faster when editing the last groups in a large document ([a69d197](https://github.com/dune3d/dune3d/commit/a69d19771e9adb57f2a5f2bf04f42f1b15bcdd48))
 - make solver faster by only adding referenced entities ([7e3d323](https://github.com/dune3d/dune3d/commit/7e3d323746ecccb821eb3f786eb6fe456116797b))
 - allow rotating step model in import tool ([4ebbb70](https://github.com/dune3d/dune3d/commit/4ebbb706910f432ee588f47ae887c0b15a2a653f))
 - omit trailing zeros on constraints ([8c17279](https://github.com/dune3d/dune3d/commit/8c172798543551952d57c8d098b816b062297561))
 - add actions for switching to specific views ([5f17d39](https://github.com/dune3d/dune3d/commit/5f17d395c3ca4b17b137e9a2a30095fdc52caa6c))
 - add go to group/source group actions ([146016c](https://github.com/dune3d/dune3d/commit/146016cf102023a04865d8bf12bde8f92aa08e38))
 - auto-rotate workplane in draw workplane tool ([a060f4d](https://github.com/dune3d/dune3d/commit/a060f4d7a14a817c6313c9f574536b48c54a17e9))
 - show workplane in tooltip for in-workplane entities ([0f0ce86](https://github.com/dune3d/dune3d/commit/0f0ce862a4f223835c8fd8fdc087d97fb9ade74b))
 - add buttons for constraining in 3D to context menu ([4de7aab](https://github.com/dune3d/dune3d/commit/4de7aaba04030aebe7dcb44badb50747ed018065))
 - add tool for converting between tangent constraints ([08d580d](https://github.com/dune3d/dune3d/commit/08d580d1c1c102d3b1dc96a98fa55cf30cb412c3))
 - show datum in constraint tooltip ([50ebe1a](https://github.com/dune3d/dune3d/commit/50ebe1ab9f1efec62f2ee53a928dd251264da3db))
 - don't show constrain angle/perpendicular/parallel tool if two lines have a horizontal/vertical constraint ([13f0212](https://github.com/dune3d/dune3d/commit/13f021208e5801927125751d62b70a4ae710b62a))
 - add paste picture tool ([511456d](https://github.com/dune3d/dune3d/commit/511456d08ec5c7501d5c7dd1e57e88d01388e575))
 - make it possible to convert constraints to measurements and vice versa ([892ddfb](https://github.com/dune3d/dune3d/commit/892ddfb5bc3e10897aeb711ecb0d1fc7f9e0ff9b))
 - never render STEP model body if STEP model is in solid model ([801273f](https://github.com/dune3d/dune3d/commit/801273f015ca7d62c3a0b98b9c818881c4533dc4))
 - don't crash when app becomes unresponsive on wayland ([cc2cb8b](https://github.com/dune3d/dune3d/commit/cc2cb8bbbfe25907b8e317ac07d1d7db75d69a0f))
 - support exporting projection for multiple bodies ([3234eee](https://github.com/dune3d/dune3d/commit/3234eee58a521b327f0c46661bf08d12499b0425))
 - make it possible to apply tangency constraints at points with more than two coincident entities ([b837dfb](https://github.com/dune3d/dune3d/commit/b837dfb6a56b26827f4869e3354b9198e66f9e20))
 - ignore selected constraints when creating new constraints ([2eb7ad1](https://github.com/dune3d/dune3d/commit/2eb7ad11528a2f7f68c93fd4f79e0cac2aeea590))
 - make it easier to hide reference workplanes by pressing delete ([b0a35fd](https://github.com/dune3d/dune3d/commit/b0a35fd16c2fa9a17b7af4d0243771053023b0e6))
 - rotate window: use X/Y/Z instead of roll/pitch/yaw ([befa0d6](https://github.com/dune3d/dune3d/commit/befa0d67c3f5c9ebb3d13264079b477016e4d8d1))
 - increase default workplane size to 100x100mm ([0e950e8](https://github.com/dune3d/dune3d/commit/0e950e81aee58fe871d3b8a04d9c3ebdfe6acbe1))
 - make point on bezier constraint work for 3D beziers ([de16619](https://github.com/dune3d/dune3d/commit/de166198906258092af356993a8b31b11cb51162))

## Bugfixes

 - support documents with non-ASCII filenames on Windows ([5bdf8ef](https://github.com/dune3d/dune3d/commit/5bdf8efe0ca8df94df8c30f61fea5575895d1e40))
 - support importing/exporting DXF files with non-ASCII filenames on Windows ([18bbd13](https://github.com/dune3d/dune3d/commit/18bbd135cfcfcfeb7de2e37f509b0d93109e5b70))
 - disallow "In solid model" STEP entities not in sketch groups ([722d498](https://github.com/dune3d/dune3d/commit/722d49880765a89ddd5079d290eb78f0968c9e3a))
 - don't re-solve document when adding aligned distance measurement ([e5398ed](https://github.com/dune3d/dune3d/commit/e5398ed87ae8120c3f227c2be53f1403e19bc569))
 - update version info bar when opening or switching documents ([3c730b6](https://github.com/dune3d/dune3d/commit/3c730b656728bfb3dea2ab227bc53838dedaad87))
 - prevent cluster entities getting from getting scaled incorrectly when opening a document in presence of ambigous constraints such as distance ([0ce9cd8](https://github.com/dune3d/dune3d/commit/0ce9cd809a6f87de35018caeafa158f07134b02d))
 - correctly export circles in clusters ([b6fbca9](https://github.com/dune3d/dune3d/commit/b6fbca98dcdac52b1d61145b6348467bd0fdc74e))
 - Pipe Groups: don't break if things aren't at the origin ([65dd1f3](https://github.com/dune3d/dune3d/commit/65dd1f3ba97e940af27887d150edfc449fe7fbb3))
 - apply relative rotation such that axes make sense ([3857591](https://github.com/dune3d/dune3d/commit/3857591138976a375d05b801a2906f5e76d24d5b))
 - don't throw exception when saving after saving from confirm close dialog ([eae0a61](https://github.com/dune3d/dune3d/commit/eae0a61a10960037960f76bc65d4edeb6ffb17bd))
 - bezier/bezier tangent symmetric constraint now works in all workplanes instead of resulting in solver errors ([7075a3d](https://github.com/dune3d/dune3d/commit/7075a3d9c0a5b01f856bf5199e14f2fa86364e34))
 - disallow entities and constraints in reference group ([3b2273c](https://github.com/dune3d/dune3d/commit/3b2273c349875a0e73f2dd67a383975f889e1ac5))
 - properly render points and some constraint icons on AMD GPUs on mac OS ([7f9c0c6](https://github.com/dune3d/dune3d/commit/7f9c0c68b23810d203c0ef11a25fa272da5ef3b6))
 - use correct mapping for roll/pitch/yaw controls in rotate tool ([09ffee8](https://github.com/dune3d/dune3d/commit/09ffee875007655081a3dec6bd94dae53cd1ed50))
 - mac: fix broken selection on intel platform ([e4df71c](https://github.com/dune3d/dune3d/commit/e4df71ca0f7be767a138eb85d90bc2f4a6ef4348))
 - add actions for creating all groups ([3e7759a](https://github.com/dune3d/dune3d/commit/3e7759a7cfefc385f4094d2681d6e85d78ed9e55))
 - persist solid model operation in sketch groups ([7237eb8](https://github.com/dune3d/dune3d/commit/7237eb81991c10b702ff4e86b69fb99ec3a25075))

# Version 1.3.0

## New Features

 - Bitmap picture import ([0c3a0bd](https://github.com/dune3d/dune3d/commit/0c3a0bd84493484b5f552ebea04c3920e2edf89f))
 - Pipe groups ([4cce3ca](https://github.com/dune3d/dune3d/commit/4cce3cafd5e16c9873db5123f3dd54182b7e500c))
 - Clone groups ([1821909](https://github.com/dune3d/dune3d/commit/18219099be0f480c39442fc6cdaf9b5b1a024b50))
 - Solid model operation group ([9d95d64](https://github.com/dune3d/dune3d/commit/9d95d6486b1a32fa4d2c4c0a35e6ca953fbadd4a))
 - Horizontal/Vertical mirror groups ([e1c4ebb](https://github.com/dune3d/dune3d/commit/e1c4ebbf59432a147bae069e46b9d553826a9e12))
 - Curvature combs for visualizing curvature ([e244c3a](https://github.com/dune3d/dune3d/commit/e244c3a03d51e339f74a7266d0f7bb1023ee9e57))
 - preview constraints on hover in context menu ([2e37a8c](https://github.com/dune3d/dune3d/commit/2e37a8c5f066012bd34ec33123b708be9fca9ac9))
 - context menu for constraining ([175ed27](https://github.com/dune3d/dune3d/commit/175ed27848d3f94da13e24cceff0f637192d5f5e))
 - Create coincident constraints tool ([2a5398e](https://github.com/dune3d/dune3d/commit/2a5398e9114ea6cf7486c0c7d53155b691092508))

## Enhancements

 - open selection menu on long click so that it also works in tools ([6e7e391](https://github.com/dune3d/dune3d/commit/6e7e39108e8c9f972a79f66f29704023cdaece08))
 - don't forget collapsed state of bodies in workspace browser ([95a2cd7](https://github.com/dune3d/dune3d/commit/95a2cd7e50d082d49d24d017ea4fa86776cf0469))
 - show keyboard shortcuts in action bar buttons tooltip ([8bcf0c9](https://github.com/dune3d/dune3d/commit/8bcf0c9b8b0917937c9d7c2411383246e595513f))
 - show keyboard shortcuts in context menu ([0cfb6b4](https://github.com/dune3d/dune3d/commit/0cfb6b474d019e4da0ed48ad02dde7f25718e73c))
 - draw rectangle tool: always add center point in center mode ([a75b1aa](https://github.com/dune3d/dune3d/commit/a75b1aa3193c11676183afb9f9c2dd43ef82413d))
 - show popup when a group can't be moved ([ffbb83e](https://github.com/dune3d/dune3d/commit/ffbb83ee3822d0fe02fc933c381dd270d815cccb))
 - drag items even if not selected ([60816ba](https://github.com/dune3d/dune3d/commit/60816ba332f4a83f168b020d3967a66ba2e8fe5b))
 - support constraining workplanes/STEP entities to 3D circles/arcs ([007f5ee](https://github.com/dune3d/dune3d/commit/007f5ee18aa6849a158d47b3339d421e28015182))
 - use nested context menu ([299ea18](https://github.com/dune3d/dune3d/commit/299ea187ab5b1f9a4130ea22bf84029962635d1d))
 - speed up document update when deleting measurements ([20542eb](https://github.com/dune3d/dune3d/commit/20542eb48cf8968c14b8b1b53edfb62db9bbd73d))
 - make it easier to create new bodies ([7b1dd70](https://github.com/dune3d/dune3d/commit/7b1dd707120e609479acdfb5edf09d0b565b33f0))
 - constrain workplane normal tool: don't create invalid normal ([681612a](https://github.com/dune3d/dune3d/commit/681612ad16d72776851d885ca5e2deed0788b0fc))
 - look here action ([d02eed3](https://github.com/dune3d/dune3d/commit/d02eed373615ab9facf232f4aaea5154fe85b282))
 - default export filename to document filename if it hasn't been exported before ([e493dd8](https://github.com/dune3d/dune3d/commit/e493dd8d06a3ed44ed2419805e94c40afa7b6166))

## Bugfixes

 - properly update selection editor after tools ([3adf525](https://github.com/dune3d/dune3d/commit/3adf525c710aa6f03d45171a2b96eab69f13aaf9))
 - don't crash on too many unknowns ([d74b1b8](https://github.com/dune3d/dune3d/commit/d74b1b8174a9d9bd610be07d1bd5e585039c9373))
 - don't crash when closing the current document while a tool is active ([369d3d0](https://github.com/dune3d/dune3d/commit/369d3d0dda6d2a79b177fdfd17f384cb1c398c69))
 - rotate tool: start solving at the correct group ([a8f1d8b](https://github.com/dune3d/dune3d/commit/a8f1d8b538bc14a3bd8d8c24093ba6d2ad6eccc4))
 - Loft Group: error out on holes ([90513bb](https://github.com/dune3d/dune3d/commit/90513bb8de89c20eda9260a434141415a792b703))
 - don't crash when switching to the constraints tab if an entry in selection or group editor had focus ([50965e4](https://github.com/dune3d/dune3d/commit/50965e4c738a8791b7e6af66935e1f03c2c02871))
 - don't throw exception when opening three documents ([2552876](https://github.com/dune3d/dune3d/commit/2552876df87ee6ee833dc2d89f5c3bbcc9c7c665))
 - properly switch workspace views when opening more than one document ([f5e4610](https://github.com/dune3d/dune3d/commit/f5e4610b5a0213da592f8d0749939c39eec037ad))
 - use correct orientation for aligned distance constraint when using workplane for direction ([6527950](https://github.com/dune3d/dune3d/commit/6527950ef79a622e0d9b6bae3d992d01b986b9d2))
 - properly generate faces for geometry close to circles ([d24e748](https://github.com/dune3d/dune3d/commit/d24e7488ae24bd80e940fa3d737a291a4cf30622))
 - wrap message popup in workspace browser and actually hide it ([ab20dfc](https://github.com/dune3d/dune3d/commit/ab20dfce4673a6bf73614eca46c81050b92ea13f))
 - fix pasting clusters with anchors ([524ef58](https://github.com/dune3d/dune3d/commit/524ef58becfea5a67dd9d24dd1b2c8fe62bf75e8))
 - prevent NaNs in view matrix resulting in everything disappearing ([9e13c6e](https://github.com/dune3d/dune3d/commit/9e13c6e6d06aac4eb0d2adade5a7be9ae80a9619))
 - don't crash when exporting STL for a group that has no solid model ([24dbc34](https://github.com/dune3d/dune3d/commit/24dbc34a7fbc9bd52f93506ffa7dc9185d265d67))
 - only update canvas once after operations ([d3e8152](https://github.com/dune3d/dune3d/commit/d3e81526a847e200fc8b7f7cddd9afacbb7d0021))
 - enable distance measurements even if entities are from previous groups ([f397d23](https://github.com/dune3d/dune3d/commit/f397d23b1d1d350edb48725ca4981fc22ffa7d7b))
 - default to trackball rotation for new installations ([d2724e1](https://github.com/dune3d/dune3d/commit/d2724e1d80a1797b80007fbd8a458e3574a85c5c))
 - make navigations buttons work for switching groups on Windows ([b9fe4e9](https://github.com/dune3d/dune3d/commit/b9fe4e94828454f2e9e249891023b9e841d76adc))

# Version 1.2.0

## New Features

 - add copy/paste ([b880274](https://github.com/dune3d/dune3d/commit/b8802743de03cb7dbeb14f64993d57f28312812f))
 - add body colors ([3f858f1](https://github.com/dune3d/dune3d/commit/3f858f158fc0a25f7f006e6f6f20eeeefe743bd6))
 - add text entities ([5dac357](https://github.com/dune3d/dune3d/commit/5dac35766a8cf86aa0fbb2a67a7c1dfc13d36b48))
 - add cluster entities ([c950380](https://github.com/dune3d/dune3d/commit/c9503801e41389b1f5b0355d40091b18e2bb2f65))
 - add DXF import ([270e757](https://github.com/dune3d/dune3d/commit/270e757acc73981a0f38b48f5967652ab754774d))
 - add bezier curves ([b4e5920](https://github.com/dune3d/dune3d/commit/b4e5920a49182dfe8c11d7d17d0b861648804d21), [aaf3e6f](https://github.com/dune3d/dune3d/commit/aaf3e6f6b5506fdc2600e8578f5d697b68e8e413))
 - add loft groups ([80450f5](https://github.com/dune3d/dune3d/commit/80450f5e0cbbb666f4fb9949e6ab37ee482366fc))
 - add revolve groups ([936aca6](https://github.com/dune3d/dune3d/commit/936aca66c6d214402d626edf27a4a04a641857f9))
 - workspace views for saving visible groups ([eeeca50](https://github.com/dune3d/dune3d/commit/eeeca50546caba676ddaaf2dd5c74acc29bad16c))
 - support linking documents as entities ([c78c9f7](https://github.com/dune3d/dune3d/commit/c78c9f79b06f41052ba7b702b8a3189a97b75840))
 - support multiple documents ([dd56445](https://github.com/dune3d/dune3d/commit/dd5644582a9b5d134db1575163d7afd7be618e34))
 - add keyboard pan/zoom/rotate ([55617b4](https://github.com/dune3d/dune3d/commit/55617b48873883dbf8e3837b27a37b3832b180df))
 - add menu for selecting obscured items ([760c851](https://github.com/dune3d/dune3d/commit/760c8510b762972c77ac0eda4ca84d5f0e400e2a))
 - add measurement constraints ([895f2b3](https://github.com/dune3d/dune3d/commit/895f2b36563c586a9f35274f7b44320fa12e30cb))

## Enhancements

 - improve rotation with new trackball scheme ([42d39ce](https://github.com/dune3d/dune3d/commit/42d39ce7fdd44f93ce71cce61e56bf1dc3967094))
 - add step model wireframe display mode ([8cc3b92](https://github.com/dune3d/dune3d/commit/8cc3b920f04315640928d271ffbf7b49263444ed))
 - show popup if a group couldn't be created ([c2cf03c](https://github.com/dune3d/dune3d/commit/c2cf03c1d735be527d853f0f65eff02a697348c4))
 - add opened documents to recent list on windows ([5032e94](https://github.com/dune3d/dune3d/commit/5032e94e20ab7e1c87bd635c166aed65e168b821))
 - show document path in header bar and workspace browser tooltip ([4fe0820](https://github.com/dune3d/dune3d/commit/4fe082083a42de88b0d0d4b31d9fe2824b52a857))
 - add tooltip for hover selection ([32155b5](https://github.com/dune3d/dune3d/commit/32155b514a17ff8143ed4ffc0da95adabcd01036))
 - make loading documents more tolerant to errors ([fcaa664](https://github.com/dune3d/dune3d/commit/fcaa664aebfa6f07665ac806acba87d960fc0297))
 - add option for showing construction entities from previous groups ([70df064](https://github.com/dune3d/dune3d/commit/70df064886264d04a62d02139c97d327f3edfc9f), [4aa0721](https://github.com/dune3d/dune3d/commit/4aa0721c66e4c15d878886a46ddd91d361078dd9))
 - show filename in window title ([160f897](https://github.com/dune3d/dune3d/commit/160f8971407540f18f0692e7fd8c0fa50b0f257f))
 - add STEP model reload button ([7203abf](https://github.com/dune3d/dune3d/commit/7203abff704111b8f20af34a51fc7ad22b569259))
 - use sensible default paths in file dialogs ([9fca904](https://github.com/dune3d/dune3d/commit/9fca904798ab9686c6228cf0c8efad3a275f5509))
 - draw contour tool: support tangent constraint on start point ([c8700c4](https://github.com/dune3d/dune3d/commit/c8700c4350c595c33216fcdd91df89f8b6bd2fab))
 - add option for hiding STEP solid model ([4fca12b](https://github.com/dune3d/dune3d/commit/4fca12b13e1f092d66cae6647c344b2eb5462f48))
 - let selected items glow, off by default ([8b2f77c](https://github.com/dune3d/dune3d/commit/8b2f77cec27476adba4cffd59362748e39875c1d))
 - automatically create tangent constraints when closing a contour ([e9bbf50](https://github.com/dune3d/dune3d/commit/e9bbf5075513ffc9b7502ac90a8bbb780255fd2d))
 - use different icons for sketch points depending on type ([f923ce0](https://github.com/dune3d/dune3d/commit/f923ce0d2cf7d095a8ac0bf726ff965cee641e07))
 - separate tool for point/line and point/plane distance constraints ([f0de1dd](https://github.com/dune3d/dune3d/commit/f0de1dd4e933794bc81370ac53d52fb02157585d))
 - put line/points perpendicular constraint in separate tool ([e6b9e64](https://github.com/dune3d/dune3d/commit/e6b9e64e7dcc0dcd7bef9160480260ff91df8fca))
 - put tangent constraints in separate tools ([8cc7da4](https://github.com/dune3d/dune3d/commit/8cc7da4c6c39a6e9147626f6c12d418031ed5c56))
 - replace Constrain coincident tool with tools for every constraint ([6272ebf](https://github.com/dune3d/dune3d/commit/6272ebf07530547366453f2d9d8f4a25fd274f96))
 - only show constrain equal radius/distance tool if just the right things are selected ([5fde27d](https://github.com/dune3d/dune3d/commit/5fde27dbeeef25fe74d01ebda88687784b3068bb))
 - only create constraints if at least one entity is in current group ([0e431c9](https://github.com/dune3d/dune3d/commit/0e431c9a0147fc489dd5e400da5f96892935d08b))


## Bugfixes

 - don't open the same document twice in two windows ([d5d2c6c](https://github.com/dune3d/dune3d/commit/d5d2c6c06db8d35d46829742c8b20b81ca90c1c0))
 - properly handle unknown item types ([37d65bd](https://github.com/dune3d/dune3d/commit/37d65bd4b585b03ab35c81e8686217aed62cb96d))
 - refresh array offset when updating group ([2f975bf](https://github.com/dune3d/dune3d/commit/2f975bfa4e914b2298adf21e30dae51a73b64963))
 - only accepts shortcuts when canvas has focus ([dffc6f0](https://github.com/dune3d/dune3d/commit/dffc6f0412b369fdaab1ba2e68ffb593b63c0a83))
 - properly reload changed STEP files without restarting the app ([07b4e8c](https://github.com/dune3d/dune3d/commit/07b4e8c3a567b87fdbffb26099b92010a92f7420))
 - allow solid model array in new body ([42963ec](https://github.com/dune3d/dune3d/commit/42963ec3ee77b4910caff434fe769b6b473c7ca0))
 - draw contour tool: properly add constraints when placing arc center ([cec8d37](https://github.com/dune3d/dune3d/commit/cec8d37f3d5110d670e0b7b3ad9504149ed3d677))
 - don't require pressing enter in group editor spin buttons ([bba8cbd](https://github.com/dune3d/dune3d/commit/bba8cbd0a6d30ffa5bbfc24f0d1b2e441b754fb4))
 - don't require pressing enter in selection editor ([f2c8c46](https://github.com/dune3d/dune3d/commit/f2c8c46879bf29fac197279bd9fbd7def6227c57))
 - properly save preferences when closing window ([55562da](https://github.com/dune3d/dune3d/commit/55562dac499784d5ba0fc6195a95646ea6d7a544))
 - increase max. distance length to 1km ([a7df8fd](https://github.com/dune3d/dune3d/commit/a7df8fd18fe98ac03fa143c4de1c0d24330257f8))

## Other changes
 - Arc/Arc tangent constraint is now called curve/curve tangent ([b821a47](https://github.com/dune3d/dune3d/commit/b821a47883e3df58e417c6e10852f93ca8f64646))

# Version 1.1.0

## New Features

 - symmetry constraints ([5437a8c](https://github.com/dune3d/dune3d/commit/5437a8c1be3d1696d55a7114a81088dc7e2ee9d1))
 - point in workplane constraint ([bc4c74f](https://github.com/dune3d/dune3d/commit/bc4c74f667e2de1576600841038ba6e7aaa5d2ac))
 - selection filter ([7e13c45](https://github.com/dune3d/dune3d/commit/7e13c45dc98a89471950ed3a6b3ad35a0e040420))
 - select all action ([a376f8e](https://github.com/dune3d/dune3d/commit/a376f8e32e29c4064a47f79186e142cc88edae02))
 - deleted items popup ([9b8aea0](https://github.com/dune3d/dune3d/commit/9b8aea0810dea6f47b9991fa5cd9b858e0d4d57c))
 - action for selecting underconstrained points ([3498e60](https://github.com/dune3d/dune3d/commit/3498e602171d67c9c90cf78a674bfcf1d62036b7))
 - filtering for redundant constraints ([c48ea16](https://github.com/dune3d/dune3d/commit/c48ea167efa27b90f272a75021fda129820ccbaa))

## Enhancements

 - link for finding redundant constraints ([bc6addb](https://github.com/dune3d/dune3d/commit/bc6addb813635b9ed27bf32c52983a68a48c91b1))
 - properly handle zero-length lines in solid model generation ([a2752fe](https://github.com/dune3d/dune3d/commit/a2752fe8ef4f50f6527d7a571d76373d28def64b))
 - zoom to cursor by default, add option for zooming to center ([6b14f51](https://github.com/dune3d/dune3d/commit/6b14f51febf5a51bc6cd7345f4cdfe62828e7e94))
 - auto-generate unique group names ([a8777b8](https://github.com/dune3d/dune3d/commit/a8777b828532cde263f1c2873caeceba7e7c423f))
 - automatically set active workplane when drawing the first workplane in a group ([3dacc3d](https://github.com/dune3d/dune3d/commit/3dacc3d8e943ff16ff43ecd47e05bcb92fb93ecb))
 - turn canvas red if there are solver errors ([1a65cd0](https://github.com/dune3d/dune3d/commit/1a65cd0c6be525c3b5c6d620fcbf85fad4b87f8a))
 - don't create redundant h/v constraints ([d024df5](https://github.com/dune3d/dune3d/commit/d024df50555fe48ca5c537665f42b4229257e3e8))
 - keep content locked to pointer when panning ([b8950de](https://github.com/dune3d/dune3d/commit/b8950de7c1fb76cc476a4b199955ecc978e2edbd))

## Bugfixes

 - STEP import: support uppercase extensions ([e3282cf](https://github.com/dune3d/dune3d/commit/e3282cf6382064b281057b9d4df5683ae8d8402b))
 - properly handle undo when toggling body ([c5754a3](https://github.com/dune3d/dune3d/commit/c5754a39d08bf343f52ec7e08cdae10938b13abe))
 - render constraint icons on zero-length lines ([618de51](https://github.com/dune3d/dune3d/commit/618de513db19eca6c45eac4bef0b90ed21c36a6a))
 - keep digits after the comma when entering angles ([73033ce](https://github.com/dune3d/dune3d/commit/73033ce86876c8d8dc6f418610015ba57d7e165a))
 - fix box selection on intel GPUs on windows ([cf38673](https://github.com/dune3d/dune3d/commit/cf38673cbdab65898bc0ff5785afe0e17e70495a))
 - properly constrain points when drawing arcs ([74095ca](https://github.com/dune3d/dune3d/commit/74095cae547bf88dc46e75661fc941c4fdbc2717))
 - fix extrude entities not converging after reloading document ([b97f544](https://github.com/dune3d/dune3d/commit/b97f5440c56ce39186864b79052c69fd1ae9b87d))
 - prevent crash when opening context menu for constraining distance ([8b51b32](https://github.com/dune3d/dune3d/commit/8b51b327c2432c1abf22e8b46e48334ae856a506))
 - prevent exception when deleting groups indirectly ([0e38d05](https://github.com/dune3d/dune3d/commit/0e38d05f3143eb01b9918953485af749761b7cbb))
 - don't throw an exception when reordering groups in unsupported ways ([a62cebe](https://github.com/dune3d/dune3d/commit/a62cebe59f15a6f0b61912416d104b80cd70f224))
 - cancel drag when pressing other mouse button ([9e0055f](https://github.com/dune3d/dune3d/commit/9e0055fd8210bb3214d9e67b707a5664e908d468))
 - fix constraint icon and text selection ([d679ead](https://github.com/dune3d/dune3d/commit/d679ead3ad5c8a5cd46e6999ee2382c498f919d1))
 - STEP import: fix things appearing at the origin ([38fb19c](https://github.com/dune3d/dune3d/commit/38fb19c626c54921373f89c19b8a8e9540e5af95))
 - support nested solids for extrusion and lathe groups ([f777807](https://github.com/dune3d/dune3d/commit/f777807b41c4dd87c25fe9cddf7e65adc06924dc))
 - don't crash when closing and re-opening log window ([f0257f2](https://github.com/dune3d/dune3d/commit/f0257f2da931efc7eadada0445f37d0a128cee1a))
 - properly handle redundant constraints in some cases ([c0e7a4c](https://github.com/dune3d/dune3d/commit/c0e7a4cd9e79ac2cc9b85088df839e05ef4d9182))

# Version 1.0.0

No change log since this is the first versioned release.
