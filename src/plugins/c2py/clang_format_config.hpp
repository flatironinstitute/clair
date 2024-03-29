
static constexpr auto clang_format_config = R"RAW(
BasedOnStyle: LLVM

AccessModifierOffset: 0
AlignAfterOpenBracket: Align
AlignConsecutiveAssignments: true
AlignConsecutiveDeclarations: false
AlignEscapedNewlinesLeft: false
AlignOperands: false
AlignTrailingComments: true
AllowAllParametersOfDeclarationOnNextLine: false
AllowShortBlocksOnASingleLine: true
AllowShortCaseLabelsOnASingleLine: true
AllowShortFunctionsOnASingleLine: All
AllowShortIfStatementsOnASingleLine: true
AllowShortLoopsOnASingleLine: true
AlwaysBreakBeforeMultilineStrings: true
AlwaysBreakTemplateDeclarations: false
BinPackArguments: true
BinPackParameters: true 
BreakBeforeBinaryOperators: NonAssignment
BreakBeforeBraces: Attach
BreakBeforeTernaryOperators: false
BreakConstructorInitializersBeforeComma: false
BreakStringLiterals: false
ColumnLimit: 200
ConstructorInitializerAllOnOneLineOrOnePerLine: true
ConstructorInitializerIndentWidth: 3
ContinuationIndentWidth: 3
Cpp11BracedListStyle: true
DerivePointerBinding : false
IndentCaseLabels: true
IndentWidth: 2
Language: Cpp
MaxEmptyLinesToKeep: 1
NamespaceIndentation : All
PointerAlignment: Right
ReflowComments: false
SortIncludes: false
SpaceAfterControlStatementKeyword: true
SpaceBeforeAssignmentOperators: true
SpaceInEmptyParentheses: false
SpacesInParentheses: false
Standard: Cpp11
TabWidth: 2
UseTab: Never

)RAW";