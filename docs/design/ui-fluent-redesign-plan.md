# Ditto 视觉现代化重设计方案（Windows 11 Fluent）

> 版本：v1.0（2026-10-02）
> 基线：上游 master（a80fd35）之后的新分支
> 风格：Windows 11 Fluent 视觉语言
> 范围：全应用，分阶段实施（P0–P4）
> 本文档是后续实施的唯一依据；实施时按第 8 节拆票推进。

---

## 目录

1. [现状盘点与问题清单](#1-现状盘点与问题清单)
2. [设计原则与视觉语言](#2-设计原则与视觉语言)
3. [设计 Token 体系与主题 Schema v4](#3-设计-token-体系与主题-schema-v4)
4. [基础设施改造](#4-基础设施改造)
5. [逐表面视觉规格](#5-逐表面视觉规格)
6. [主题兼容与深浅色策略](#6-主题兼容与深浅色策略)
7. [DPI 与可达性](#7-dpi-与可达性)
8. [分阶段实施计划、验收与风险](#8-分阶段实施计划验收与风险)
9. [明确不做的事](#9-明确不做的事)

---

## 1. 现状盘点与问题清单

### 1.1 结论速览

| 维度 | 现状 | 评价 |
|---|---|---|
| 公共控件视觉样式 | comctl32 v6 已启用（`src/StdAfx.h:43` manifestdependency） | ✅ 底子是好的 |
| DPI 感知 | Per-Monitor V2（`DeclareDPIAware.manifest` + vcxproj 双声明），自建 `CDPI`（`src/DPI.h`），全库 102 处 `Scale()` | ✅ 已达标，缺口见 1.4 |
| 绘制技术 | 纯 GDI 自绘 + GDI+（仅 PNG 解码/图片查看/QR），无 D2D/DWrite | 可沿用，不迁移 |
| 主题引擎 | `CTheme`（`src/Theme.h`）≈30 个颜色 + 2 个尺寸，XML 格式 Version=3，12 个主题文件 | 可扩展为 v4 |
| 主窗口 | 自绘标题栏/列表/搜索框/滚动条，已有主题接入 | 有底座，细节旧 |
| 对话框 | 全部 `MS Shell Dlg 8pt` 固定 rc 坐标，几乎未接主题 | ❌ XP 感重灾区 |
| 字体 | rc 写死 MS Shell Dlg / MS Sans Serif；运行时硬编码 Segoe UI；无统一字体服务 | ❌ 三分裂 |
| 图标资源 | 158 个多 DPI PNG（GDI+ 解码，`GdiImageDrawer` 按 DPI 选档）+ 旧 BMP 工具栏 | ✅ 管线可沿用 |

**核心判断：Ditto 不缺视觉基础设施（comctl6 / PMv2 / GDI+ / 主题引擎 / 自绘控件全家桶都在），缺的是一套统一的设计 token、一套字体服务、和对 14+ 个对话框的主题化覆盖。"XP 感"主要来自对话框层的字体与固定布局，以及主窗口的状态细节（hover、圆角、死字段），而不是来自缺现代框架。**

### 1.2 主窗口（CQPasteWnd）

结构：`CQPasteWnd : CWndEx`（自绘非客户区，内嵌 `CDittoWindow`），控件见 `QPasteWnd.h:130-147`：

- 列表 `CQListCtrl`：`LVS_REPORT|LVS_OWNERDATA|LVS_OWNERDRAWFIXED|LVS_NOCOLUMNHEADER`，NM_CUSTOMDRAW 全接管（`QListCtrl.cpp:430-658`，返回 `CDRF_SKIPDEFAULT`）
- 搜索框 `CSymbolEdit : CEdit`，完全自绘（`SymbolEdit.cpp:459-574`）
- 分组树 `CGroupTree : CTreeCtrl`，`TVS_HASLINES|TVS_LINESATROOT|TVS_HASBUTTONS` 经典样式（`QPasteWnd.cpp:456`）
- 自绘滚动条 `CModernScrollBar`（主题色，已现代化）
- GDI+ PNG 图标按钮 `CGdipButton` × 3（分组/返回/系统菜单）

问题清单（按视觉影响排序）：

| # | 问题 | 证据 |
|---|---|---|
| M1 | 快捷键角标用古董点阵字体 "Small Font" | `QListCtrl.cpp:2310` |
| M2 | 搜索框自绘先刷 `GetSysColor(COLOR_WINDOW)`，深色主题下白闪 | `SymbolEdit.cpp:473` |
| M3 | 分组树完全未接主题：`Theme.h:42-43` 的 `GroupTreeBG/GroupTreeText` 全库无调用方；深色下原生灰底与窗口割裂；`TVS_HASLINES` 树线是 XP 标志物 | `QPasteWnd.cpp:456` |
| M4 | 列表无 hover 态：`OnMouseMove` 只用于滚动条检测，未 `TrackMouseEvent`；选中态是纯色矩形填充，无圆角、无 focus 区分 | `QListCtrl.cpp:2101` 起、`483-509` |
| M5 | 标题栏 = 左右两块纯色 `FillRect` 拼接，无圆角、无 DWM 暗色标题栏、无材质 | `DittoWindow.cpp:403-410` |
| M6 | 字体硬编码：搜索框 Segoe UI 15pt、分组标题 Segoe UI 12pt 下划线（`QPasteWnd.cpp:2214,2221`）；布局魔法数（搜索行 `searchRowStart=33`、按钮 24×24，`QPasteWnd.cpp:667-785`） | 同左 |
| M7 | 主列表无双缓冲（`memdc.h` 全库仅 `GdipButton.cpp:239`、`ImageViewer.cpp:122` 两处在用），滚动可能闪烁 | `QListCtrl.cpp` OnEraseBkgnd `1214-1239` |
| M8 | ~~`WM_DPICHANGED` 仅编辑窗链处理（`EditFrameWnd.cpp:45`），QPasteWnd 未注册，跨屏拖动不即时缩放~~ **条目过时（2026-10-03 核对）**：merge-base 的 `QPasteWnd.cpp:322` 上游已有处理器，361562a 移除的是本项目的重复注册 | `QPasteWnd.cpp:322` 附近 |
| M9 | 硬编码颜色残留：置顶警告条黄底蓝字 `RGB(255,255,0)/RGB(0,0,255)`、prompt 灰 `RGB(127,127,127)` | `QPasteWnd.cpp:510-511`、`SymbolEdit.cpp:22` |
| M10 | 主题模型无 hover/边框/圆角/字体 token；部分主题文件缺新节点时回退到 LoadDefaults 灰阶，深色主题出现浅色兜底 | `Theme.cpp:23-72` |

### 1.3 对话框与二级窗口（XP 感重灾区）

在用表面完整清单（重设计范围依据）：

**Options 属性表**（`COptionsSheet : CPropertySheet`，`EnableStackedTabs(TRUE)` 多行堆叠 tab，`OptionsSheet.cpp:48`；启用 Friends 时 8 页，否则 7 页）：

| 页 | 类 | rc 资源 |
|---|---|---|
| General | `COptionsGeneral` | IDD_OPTIONS_GENERAL (2003)，`CP_Main.rc:892` |
| Supported Types | `COptionsTypes` | IDD_OPTIONS_TYPES (2004)，rc:770 |
| Keyboard Shortcuts | `COptionsKeyBoard` | IDD_OPTIONS_KEYSTROKES (2001)，rc:827 |
| Copy Buffers | `COptionsCopyBuffers` | IDD_OPTIONS_COPY_BUFFERS (166)，rc:1109 |
| QuickPaste Keyboard | `CQuickPasteKeyboard` | IDD_OPTIONS_QUICK_PASTE_KEYBOARD (249)，rc:1216 |
| Friends（条件） | `COptionFriends` | IDD_OPTIONS_FRIENDS (148)，rc:1046 |
| Stats | `COptionsStats` | IDD_OPTIONS_STATS (132)，rc:942 |
| About | `CAbout` | IDD_ABOUT (136)，rc:1025 |

**独立模态/无模态对话框**：

| 类 | 用途 | 备注 |
|---|---|---|
| `CAdvGeneral` | 高级选项，`CMFCPropertyGridCtrl` 属性网格（~47 项），rc 唯一用 Segoe UI 10pt 的页 | `AdvGeneral.h:32` |
| `CCopyProperties` | 单条剪贴项属性 | 高频 |
| `CDeleteClipData` | 批量删除（大 ListCtrl，789×315） | 高频 |
| `CMoveToGroupDlg` / `CGroupName` / `CAddType` | 组操作、添加格式 | 高频小框 |
| `CFriendPromptDlg` / `CFriendDetails` | 发送给好友 / 好友编辑 | 网络功能 |
| `CGlobalClips` | 全局热键列表（无模态） | |
| `CScriptEditor` | ChaiScript 脚本管理 | |
| `CFileTransferProgressDlg` | 传输进度，`CAnimateCtrl` 播放 FILECOPY.AVI | AVI 需替换 |
| `CEditFrameWnd`/`CEditWnd` | 剪贴项编辑器窗口链（自绘标题栏 + `CTabCtrlEx` + RulerRichEdit） | |
| `CToolTipEx` | 富文本描述悬浮窗（含 `CImageViewer`） | |
| `CQRCodeViewer` / `CProgressWnd` / `CNoDbFrameWnd` | 二维码 / 删除进度 / 无数据库窗 | |

**系统性问题**：

| # | 问题 | 证据 |
|---|---|---|
| D1 | 几乎全部对话框 `FONT 8, "MS Shell Dlg"`；`IDD_DIALOG_REMOTE_FILE` 更老，`MS Sans Serif` | `CP_Main.rc:749` 等 20+ 处、rc:1099 |
| D2 | 布局固定：rc 的 `AFX_DIALOG_LAYOUT` 段全为 0，MFC 自动布局关闭；自研 `CDialogResizer` 仅 5 个对话框在用（CopyProperties/AdvGeneral/DeleteClipData/GlobalClips/ScriptEditor） | `DialogResizer.h:14-40` |
| D3 | 无任何对话框接主题（OnCtlColor 多为空实现或已注释），亮色系统灰 + 深色主题主窗口并存 | `OptionsGeneral.cpp:457-467` 等 |
| D4 | 字体三分裂：MS Shell Dlg（rc 主体）/ Segoe UI（仅 Adv 页 + 窗口类硬编码）/ MS Sans Serif（2 处遗留） | 见 1.2/1.3 |
| D5 | 旧新图标并存：多 DPI PNG 新体系 vs `Toolbar.bmp`/`back.bmp` 等 BMP 旧资产 | `CP_Main.rc:83-119` |
| D6 | `CHyperLink` 是 1997 年 Chris Maunder 实现（蓝/紫/hover 手绘下划线） | `HyperLink.h:20` |
| D7 | 死代码对话框 4 个：`COptionsQuickPaste`、`COptionsUtilities`、`CSelectDB`、`CDittoPopupWindow`（无实例化点） | 只标记，见第 9 节 |

### 1.4 可复用资产（新方案的挂点）

- `CTheme` XML 引擎：`rgb()/hsl()/#RRGGBB` 解析、`LoadWindowsAccentColor()` 跟随系统强调色、文件热重载（LastWriteTime 检查）、单例 `CGetSetOptions::m_Theme`
- 深色模式检测：`DarkAppWindows10Setting()`（`Misc.cpp:1561`）+ `MainFrm.cpp:855` 定时器轮询，已能自动切主题
- `CDittoWindow` 自绘标题栏（被 QPasteWnd/CToolTipEx/QRCodeViewer/CEditFrameWnd 共用）——单点改造全链受益
- `CModernScrollBar`、`CGdipButton`+`GdiImageDrawer` 多 DPI PNG 管线、`CDimWnd` 模态压暗、`CToolTipEx`
- `CMemDCEx` 双缓冲（`memdc.h`）、`CDPI`（`DPI.h`，GetDpiForWindow 三级回退）

---

## 2. 设计原则与视觉语言

以 Windows 11 Fluent（WinUI 3 视觉规范）为参照系，但用 MFC + GDI/GDI+ 实现，**只在能力范围内追求"神似"**：圆角、层次、字阶、accent、状态反馈五件事做到位，材质（Mica）作为可选加分项。

### 2.1 五条原则

1. **密度优先**：Ditto 是高频效率工具，列表必须保持紧凑（compact 单行 30px 逻辑高度为默认档），装饰为信息服务。
2. **深浅色等权**：每个 token 都定义 Light/Dark 两套默认值；不允许"深色主题下残留亮色控件"（这是当前最刺眼的问题，见 M2/M3）。
3. **状态可见**：所有可交互元素必须有 rest / hover / pressed / selected / disabled / focus 六态；键盘焦点必须有 FocusRing。
4. **跟随系统**：强调色跟随 Windows 强调色（复用 `LoadWindowsAccentColor` 链路），深浅色跟随系统模式，圆角/暗标题栏按 OS 能力降级。
5. **最小惊奇**：布局结构不推翻——主窗口仍是"标题栏 + 搜索 + 列表（+ 可关侧栏）"，Options 仍是 8 页内容，迁移成本低、用户肌肉记忆不破坏。

### 2.2 视觉语言规格（逻辑像素，96dpi 基准，全部经 `CDPI::Scale`）

| 属性 | 规格 |
|---|---|
| 网格 | 4px 基础网格；控件间距 8/12/16/24 四档 |
| 圆角 | 窗口 8px（DWM）；卡片/输入框 4px；胶囊（chip/导航选中）full round = 高度/2 |
| 阴影 | 不自绘阴影；悬浮窗（ToolTipEx/QR/弹层）用 DWM 边框 + Divider 描边代替 |
| 描边 | 卡片与输入框 1px Stroke；悬浮层 1px Divider |
| 字阶 | Caption 12 / Body 14 / Body Strong 14(Semibold) / Subtitle 16 / Title 20（见 §4.2） |
| 动效 | 仅两处：列表 hover/选中底色切换 ≤80ms 无动画（直接换色）；保存到组动画保留现状。不引入通用动画系统 |
| 图标 | 单色描边风（1.5px stroke，24 网格），运行时 tint 到 Text/Accent 色；沿用 per-DPI PNG 管线 |
| 声音/触觉 | 不涉及 |

### 2.3 主窗口布局（目标态）

```
┌──────────────────────────────────────────────────┐
│ Ditto                    [置顶] [菜单] [─][□][×] │ ← 自绘标题栏 32px（DWM 圆角 8px + 暗/亮标题栏）
├──────────────────────────────────────────────────┤
│ 🔍 搜索剪贴历史                              ✕  │ ← 搜索行 40px（搜索框圆角 4px）
├────────────┬─────────────────────────────────────┤
│ 侧栏(可关) │ [全部] [文本] [图片] [文件] [链接]  │ ← 类型 chips 行 32px
│            ├─────────────────────────────────────┤
│ ▐ 全部    │ 1  剪贴文本内容……           12:04  │
│   文本    │ 2  [缩略图] 图片截图……       11:58  │
│   图片    │ 3  <folder> 项目资料……       11:40  │
│   收藏    │ …                                    │
│ ────────  │                                     │
│ ▘ 分组树  │                                     │
├────────────┴─────────────────────────────────────┤
│ （状态角标：连接状态 / 选中计数，按需显示）        │
└──────────────────────────────────────────────────┘
```

- 侧栏为**可关**选项（注册表持久化），关掉后回到"搜索 + chips + 列表"单列结构（与上游行为兼容）。
- chips 行仅在有侧栏时折叠进侧栏顶部；无侧栏时显示在列表上方。避免两套布局各养一套代码。

> 注：上图为结构示意。侧栏/chips 是上游 master 没有的新组件，实施时作为新控件（`CSidebar`、`CChipBar`）从零编写，不移植旧 fork 分支代码。

---

## 3. 设计 Token 体系与主题 Schema v4

### 3.1 Token 命名与语义（v4 新增/映射）

现有 `CTheme` 的扁平颜色字段升级为语义分层。**旧字段全部保留 getter（现有调用点不动），内部改为从 token 派生**：

| Token（v4） | 语义 | Light 默认 | Dark 默认 | 旧字段映射（v3 → v4） |
|---|---|---|---|---|
| `Surface.Base` | 窗口底色 | `#F3F3F3` | `#202020` | MainWindowBG |
| `Surface.Elevated` | 卡片/输入框/悬浮层底 | `#FFFFFF` | `#2B2B2B` | SearchTextBoxFocusBG、DescriptionWindowBG |
| `Surface.RowAlt` | 列表奇偶行交替 | `#F9F9F9` | `#262626` | ListBoxOddRowsBG/EvenRowsBG |
| `Text.Primary` | 主文本 | `#1A1A1A` | `#FFFFFF` | ListBoxOddRowsText/EvenRowsText、GroupTreeText |
| `Text.Secondary` | 次要文本（时间/计数/prompt） | `#616161` | `#C8C8C8` | 新增（替换 SymbolEdit prompt 灰） |
| `Text.Disabled` | 禁用 | `#9D9D9D` | `#717171` | 新增 |
| `Text.OnAccent` | accent 底上的文字 | `#FFFFFF` | `#000000` | 新增 |
| `Accent.Default` | 强调色 | 系统强调色，兜底 `#005FB8` | 系统强调色，兜底 `#4CC2FF` | 沿用 LoadWindowsAccentColor |
| `Accent.Subtle` | accent 12% 透明合成（选中胶囊底） | 派生 | 派生 | 新增（替代 ListBoxSelectedBG 硬值） |
| `Accent.Text` | accent 前景（链接/命中高亮） | 派生（对 Base 保持 AA 对比度） | 派生 | SearchTextHighlight |
| `Stroke.Card` | 输入框/卡片描边 | `#E5E5E5` | `#3D3D3D` | 新增（SearchTextBoxFocusBorder 语义化） |
| `Stroke.Divider` | 分隔线/引导线 | `#EDEDED` | `#333333` | 新增 |
| `Control.Fill` | 控件静态底 | `#FBFBFB` | `#2D2D2D` | 新增 |
| `Control.Hover` | 悬停底 | `#F0F0F0` | `#383838` | 新增 |
| `Control.Pressed` | 按下底 | `#EDEDED` | `#3A3A3A` | 新增 |
| `Control.DisabledBG` | 禁用底 | `#F5F5F5` | `#292929` | 新增 |
| `State.SelectedBG` | 选中底（= Accent.Subtle） | 派生 | 派生 | ListBoxSelectedBG / SelectedNoFocusBG（无焦点版 = 再降饱和） |
| `State.SelectedText` | 选中文字 | Text.Primary | Text.Primary | ListBoxSelectedText / SelectedNoFocusText |
| `State.Hover` | 列表行悬停 | `#EAEAEA`(8% 叠加) | `#FFFFFF`(6% 叠加) | 新增 |
| `Caption.*` | 标题栏三态（Normal/TopMost/NotConnected）× 左右渐变 + 文字 | 保留机制 | 保留机制 | CaptionLeft/Right/… 全系 |
| `Indicator.Pasted` | 已粘贴标记条 | Accent.Default | Accent.Default | ClipPastedColor |
| `Indicator.Badge` | 快捷键角标文字 | Text.Secondary | Text.Secondary | ListSmallQuickPasteIndexColor |

尺寸/形状 token（v4 新增 int 字段，沿用 `LoadInt`）：

| Token | 默认 | 说明 |
|---|---|---|
| `Radius.Window` | 8 | DWM 圆角（仅提示 DWM API，实际枚举固定值） |
| `Radius.Control` | 4 | 输入框/按钮/卡片 |
| `RowHeight.Compact` | 30 | 单行模式行高 |
| `RowHeight.Comfortable` | 44 | 含缩略图/多行模式行高 |
| `CaptionSize` / `CaptionFontSize` | 32 / 14 | 现有字段语义保留（旧默认 25/19 调整为新规格） |

> **不做**通用样式 token（边框风格/动画时长/阴影参数）——用不上的可配置性一律不加。

### 3.2 Schema v4 XML 示例

解析端完全复用 `Theme.cpp` 的 `LoadElement/LoadColor/LoadInt` 骨架，只增不改。新节点缺失时回退顺序：**主题文件 → LoadDefaults（按深浅色取 Light/Dark 默认）→ 从旧字段派生**。

```xml
<Ditto_Theme_File Version="4" Author="Ditto" Notes="Fluent Light">
  <!-- v3 字段：继续可写，读取后映射进 v4 token（见 3.1 表） -->
  <CaptionLeft>#F3F3F3</CaptionLeft>
  <MainWindowBG>#F3F3F3</MainWindowBG>

  <!-- v4 新增：语义 token -->
  <Surface_Base>#F3F3F3</Surface_Base>
  <Surface_Elevated>#FFFFFF</Surface_Elevated>
  <Text_Primary>#1A1A1A</Text_Primary>
  <Text_Secondary>#616161</Text_Secondary>
  <Accent_Default>accent</Accent_Default>          <!-- "accent" 关键字 = 跟随系统强调色 -->
  <Stroke_Card>#E5E5E5</Stroke_Card>
  <Stroke_Divider>#EDEDED</Stroke_Divider>
  <Control_Hover>#F0F0F0</Control_Hover>
  <State_Hover>#EAEAEA</State_Hover>

  <!-- v4 新增：字体与形状（int / string） -->
  <Font_Family>auto</Font_Family>                  <!-- "auto" = 启动实测选定（见 4.2） -->
  <Radius_Control>4</Radius_Control>
  <RowHeight_Compact>30</RowHeight_Compact>
</Ditto_Theme_File>
```

### 3.3 派生色计算

`Accent.Subtle`、`Accent.Text`、无焦点降饱和选中色等派生 token 在 `CTheme` 内计算，不入 XML：

- 合成：`Alpha over` 逐通道混合（accent @12% over Surface.Base）；
- 对比度：`Accent.Text` 在 Accent.Default 底上按 WCAG 相对亮度校验，不达 4.5:1 时在黑/白间取对比更高者（`Text.OnAccent` 同理）；
- 系统强调色变更（`MainFrm.cpp:855` 已有轮询）触发 token 重派生，无需重读 XML。

---

## 4. 基础设施改造

新增 4 个模块（均为纯新增文件，需加入 `CP_Main.vcxproj` 与 `.filters`——**MSBuild 不 glob**）。

### 4.1 `src/DwmTheme.h/.cpp` — DWM 能力封装

```
namespace DwmTheme {
  bool ApplyRoundedCorners(HWND, bool enable);   // DWMWA_WINDOW_CORNER_PREFERENCE(33)，圆角=2(DONOTROUND=1)
  bool ApplyDarkCaption(HWND, bool dark);        // DWMWA_USE_IMMERSIVE_DARK_MODE(20)；Win10 1809 用备选值 19 探测
  bool ApplyBackdrop(HWND, Backdrop kind);       // DWMWA_SYSTEMBACKDROP_TYPE(38)：Mica=2/Acrylic=3；Win11 22H2+
  bool CanUseBackdrop();                         // RtlGetVersion >= 10.0.22621
}
```

- 全部按函数返回值判断成败，失败**静默回退**（Win10：无圆角、无 Mica，但 `ApplyDarkCaption` 在 1809+ 仍可用）；
- Mica 前提是 `DwmExtendFrameIntoClientArea` + 客户区对应区域透明绘制，与现有 GDI `FillSolidRect` 擦除冲突，**主窗口默认不开 Mica**，做成注册表开关（默认关），文档标注为实验特性；
  **实施记录（2026-10-03）**：开关已接线——注册表值 `MicaBackdrop`（默认 0），`CDittoWindow::DoCreate` 里 opt-in 时 `DwmTheme::ExtendFrame` + `ApplyBackdrop(Mica)`；材质只在客户区未被不透明擦除覆盖处可见（标题栏/边距），粗糙边缘即实验属性本身。
- 暗色标题栏开启后，自绘 `CDittoWindow` 的文字/按钮色同步取 `Caption.*` token（两者叠加时以 token 为准重绘文本）。

### 4.2 `src/Fonts.h/.cpp` — 字体服务

```
class AppFonts {
  static AppFonts& Inst();
  void Init(int dpi);                        // 启动/WM_DPICHANGED 时重建
  CFont* Get(FontToken t);                   // Caption12/Body14/BodyStrong14/Subtitle16/Title20
  const LOGFONT& BaseFont() const;
  void ApplyToChildren(CWnd* pWnd);          // EnumChildWindows + WM_SETFONT（含重绘）
  CString ResolvedFamily() const;            // 实测选定的家族名，供主题 XML "auto" 回显
};
```

- **家族回退链（启动实测）**：`Segoe UI Variable` → `Segoe UI` → `Microsoft YaHei UI`。判定方式：对候选字体 `GetGlyphIndices` 测 `U+4E2D`（中）、`U+00E9`（拉丁扩展）均有字形且 `GDI_ERROR` 不存在则选定；`Segoe UI Variable` 仅 Win11 存在，探测失败自然落到 Segoe UI。
- 字号 ramp（96dpi）：Caption 12 / Body 14 / Body Strong 14（FW_SEMIBOLD）/ Subtitle 16 / Title 20，全部 `CDPI::Scale` 后 `CreateFont`（负高度）。
- `Font_Family` token 值为 `auto` 时走回退链，主题可显式指定家族名（老用户锁定字体的口子）。
- 替换点（实施时逐一处理）：
  - `QPasteWnd.cpp:2214,2221`（硬编码 Segoe UI 15/12pt）→ `AppFonts::Get(Body/Subtitle)`
  - `QListCtrl.cpp:2310`（"Small Font"）→ `AppFonts::Get(Caption)`
  - `DittoWindow.cpp:719-726`（标题栏字体）→ `AppFonts::Get(BodyStrong)` + `CaptionFontSize` token
  - 各对话框 rc 的 MS Shell Dlg → 运行时由基类下发（见 4.3），rc 声明不再逐个改（改 rc 反而破坏 DLU 基准）

### 4.3 `src/FluentDialog.h/.cpp` — 对话框基类（最高杠杆点）

```
class CFluentDialog : public CDialog {
  //OnInitDialog 尾部调用链条：
  // 1) AppFonts::ApplyToChildren(this) —— 字体统一，DLU 布局随字体度量自然放松
  // 2) ApplyThemeBackground() —— OnEraseBkgnd 刷 Surface.Base；OnCtlColor 返回主题刷子：
  //    静态文本→Text.Primary、禁用控件→Control.DisabledBG/Text.Disabled、只读 Edit→Surface.Elevated
  // 3) AttachResizer() —— 子类可选挂 CDialogResizer（默认开：OK/Cancel 锚右下，列表/树 SizeWidth|SizeHeight）
  // 4) WM_DPICHANGED → 按比例缩放全部子控件矩形 + AppFonts::Init(dpi) 重建 + ApplyToChildren
};
class CFluentPropertyPage : public CPropertyPage { /* 同上四件 */ };
```

- **迁移方式 = 换基类**：`class COptionsGeneral : public CDialog` → `public CFluentDialog`（`CPropertyPage` 同理），现有 DDX/消息映射不动。改基类后删除各对话框里已有的空 `OnCtlColor`（D3 清理）。
- rc 里的 `FONT 8, "MS Shell Dlg"` **保留不动**：模板字体只决定初始 DLU→px 换算，运行时 `WM_SETFONT` 覆盖显示字体；这样避免逐个改 20+ 处 rc 引发的布局回归。
- 按钮：对话框内默认按钮样式交由 `CFluentButton`（4.4）逐个替换 `IDOK`/主要动作按钮；次要按钮先保持 comctl6 原生样式（v6 下已是 Win11 视觉），不强制全换。
- **resize 行为统一**：现状仅 5 个对话框可缩放；基类默认给"右下角按钮组锚定 + 内容区拉伸"的通用 resizer 配置，个别对话框（如 About）关闭缩放。

### 4.4 `src/FluentButton.h/.cpp` — 三变体按钮

```
class CFluentButton : public CButton {   // BS_OWNERDRAW
  enum class Style { Accent, Secondary, Subtle };
  // Accent:    Accent.Default 填充圆角 4px + Text.OnAccent；hover 加亮 8%，pressed 加深 8%
  // Secondary: Control.Fill 底 + 1px Stroke.Card 描边；hover Control.Hover
  // Subtle:    透明底；hover Control.Hover（工具栏式）
  // 禁用：Control.DisabledBG + Text.Disabled；焦点：1px FocusRing 内描边（accent）
};
```

- GDI+ `Graphics::FillPath` 圆角矩形 + `AppFonts::Get(Body)` 文本；双缓冲用 `CMemDCEx`；
- 用于：对话框 IDOK/主操作（Accent）、Options 外壳导航（Subtle 选中态）、主窗口标题栏图标按钮（Subtle，40×32 hover 底）。

### 4.5 列表双缓冲与绘制管线

- `CQListCtrl::OnEraseBkgnd` 改 `CMemDCEx` 缓存整幅背景；NM_CUSTOMDRAW 的行绘制不变（item 级裁剪继续生效）；
- 缩略图缓存：`CBitmapHelper::DrawDIB` 结果按 clip id + 尺寸缓存到 `CImageList` 风格的 CDC 池（上限 ~200 条 LRU），当前每次 paint 重采样 DIB 是滚动卡顿源；
- 搜索命中高亮继续走 `DrawHTML`（`QListCtrl.cpp:596-604` 现链路），高亮色改取 `Accent.Text` token。

---

## 5. 逐表面视觉规格

### 5.1 主窗口标题栏（改 `CDittoWindow`，全链受益）

| 项 | 规格 |
|---|---|
| 高度 | `CaptionSize` token（32 逻辑px，旧默认 25 作废） |
| 形状 | DWM 8px 圆角（`DwmTheme::ApplyRoundedCorners`）；描边由 DWM 主题边框承担，`Border*` token 仅在圆角不可用时绘制 1px 边框 |
| 暗色联动 | 主题为 Dark 系时 `ApplyDarkCaption(hwnd, true)`，保证系统级阴影/贴边分屏的标题栏配色一致 |
| 内容 | 左：Ditto 图标 16px + "Ditto"（BodyStrong）；连接状态沿用现有三态色（Normal/TopMost/NotConnected 渐变对），NotConnected 时文字加 "未连接" 后缀（走语言 XML） |
| 按钮 | [置顶][菜单][最小化][最大化][关闭] = CFluentButton Subtle 40×32，图标用现有 PNG（close_8~16 等），hover 底 `Control.Hover`，关闭按钮 hover 用 `#E81123`（Windows 惯例，非 token） |
| 实现 | 仍走 `CDittoWindow::DoNcPaint` 自绘（NcPaint 里画按钮），DWM 层只负责圆角/暗色，两不冲突 |

### 5.2 搜索行

| 项 | 规格 |
|---|---|
| 容器 | 行高 40px，背景 Surface.Base；下缘 1px Stroke.Divider 与列表区分隔 |
| 输入框 | 占满行内边距（左右 12px）；高 28px；圆角 4px；底 Surface.Elevated；1px Stroke.Card |
| 状态 | focus：描边变 Accent.Default（2px 视觉 = 1px 描边 + 1px FocusRing 内叠）；有内容：右侧 ✕ 清除钮（Subtle） |
| 文字 | Body 14；prompt（"搜索…"）Text.Secondary；**删除 `GetSysColor(COLOR_WINDOW)`**（M2），背景一律 token |
| 历史按钮 | 保留现有下拉历史 PNG 图标，色 tint 到 Text.Secondary |

### 5.3 列表 `CQListCtrl`

| 项 | 规格 |
|---|---|
| 行高 | 两档：Compact 30 / Comfortable 44（token）；现有 `SetNumberOfLinesPerRow(1-5)` 多行机制保留，行高 = 字体实测高 × linesPerRow + 上下 padding（4px），token 只约束单行档 |
| 奇偶行 | Surface.RowAlt 交替（比现行更弱化，仅 2-3% 亮度差） |
| hover | `TrackMouseEvent` + 自维护 hoverIndex；底色 `State.Hover`；hover 且选中时仍显选中色。鼠标移出/列表重排/滚动即清除（防残影） |
| 选中 | 圆角胶囊：左右 inset 4px、上下 inset 1px、radius 4px；底 Accent.Subtle；文字 Text.Primary；无焦点降饱和 50%。废弃整行纯色矩形（M4） |
| 已粘贴标记 | 左缘 3px Accent.Default 竖条，位于胶囊外（inset 后仍贴行左缘 1px） |
| 缩略图 | Comfortable 档：32×32 图标区 + 4px 圆角裁剪 + 1px Stroke.Card；走 4.5 缓存 |
| 符号图标 | `<group>/<sticky>/<shortcut>/…` 现有 PNG 保留，绘制前按 `Text.Primary` tint（单色化）；文件夹等彩色语义图标例外 |
| 快捷键角标 | 0-9 编号：`AppFonts::Get(Caption)` + `Indicator.Badge` 色；编号与内容间竖线改 1px Stroke.Divider、高 12px（M1） |
| 命中高亮 | 搜索词着色 = Accent.Text（粗体不加） |
| 空状态 | 搜索无结果：列表中央 40px 灰图标 + "无匹配剪贴"（Caption/Secondary 两行）；复用 `m_noSearchResultsStatic` 改为自绘层 |
| 滚动条 | `CModernScrollBar` 保留，三色对齐 Stroke.Divider/Control.Hover token |

### 5.4 侧栏（新组件 `CSidebar`，可关）

- 宽 200 逻辑px，背景 Surface.Base，右缘 1px Stroke.Divider；
- 顶部"视图"段：全部 / 文本 / 图片 / 文件 / 链接 / 收藏 —— 单选，选中态 = Accent.Subtle 胶囊 + 左缘 3px Accent 竖条；图标 16px 单色描边 + tint；
- 下部"分组"段：分组树**自绘**（不再用 SysTreeView32 原生观感）：行高 28px、去 `TVS_HASLINES|TVS_HASBUTTONS`、缩进引导线用 1px Stroke.Divider 点线、展开钮自绘 12px chevron、hover Control.Hover、选中 Accent.Subtle 胶囊；背景/文字直接吃 `Surface.Base`/`Text.Primary`（终结死字段 M3）；
- 底部"新建分组"入口（Subtle 按钮）；
- 显隐：标题栏[菜单]或右键菜单开关，注册表持久化；关闭时 `MoveControls` 单列布局。

> 若实施中发现自绘树工作量超预算，降级方案：保留 CTreeCtrl 但 `TVS_TRACKSELECT`+`TVS_FULLROWSELECT`+`SetBkColor(SetTextColor)` 接 token，去 HASLINES——视觉可达 80%，工作量 20%。两案都写进票里，实施时定。
>
> **实施记录（2026-10-03）**：已采用降级案（PR #48：树为 child 窗口停靠侧栏，`TVS_TRACKSELECT`+`TVS_FULLROWSELECT` 接 token、去 HASLINES）；自绘树不再是待办。

### 5.5 类型 chips（新组件 `CChipBar`）

- 高 32px 行，chip 高 24px、胶囊圆角、内边距 10px；
- rest：Surface.Elevated 底 + 1px Stroke.Card + Text.Secondary；选中：Accent.Subtle 底 + Accent.Default 描边 + Text.Primary；计数徽标 Text.Secondary 12px；
- 无侧栏时显示；类型集合 = 文本/图片/文件/链接/其他（与现有 SQL 类型过滤能力对齐，纯 UI 层）。

### 5.6 Options 外壳（改造 `COptionsSheet`）

- **结构**：`EnableStackedTabs(FALSE)` + 隐藏原生 tab ctrl；左侧新增 200px 导航列（自绘，复用 CSidebar 的行绘制代码）：条目 = 8 页图标 + 名称；选中态同侧栏；右侧为页面容器（现有 CPropertyPage 原位嵌入，坐标平移）；
- **页面迁移零改动**：8 个页面类不动 DDX、不动 rc，仅换基类 `CFluentPropertyPage`（字体/主题/resizer 自动获得）；
- 窗口本体：DWM 圆角 + 自绘标题栏（复用 CDittoWindow）+ 暗/亮标题栏；默认尺寸 900×640 逻辑px（现 rc 尺寸过小）；
- **Adv 页属性网格**：`CMFCPropertyGridCtrl` 不引全局 VisualManager，用 `SetCustomColors`/`SetGroupTextColor` 等实例级接口对齐 token（背景 Surface.Elevated、组标题 Text.Secondary、描述 Text.Secondary）；
- 高级选项入口现状是 General 页按钮 + `CDimWnd` 压暗（`OptionsGeneral.cpp:451-454`），保留此交互（已是现代感交互）。

### 5.7 高频对话框（10 个，P3 范围）

统一动作：换 `CFluentDialog` 基类 + IDOK 换 `CFluentButton(Accent)` + 取消换 `Secondary` + 挂 resizer。逐个附加项：

> **实施记录（2026-10-03）**：按钮接线已集中到 `CFluentDialog::WireFluentButtons()`（基类 OnInitDialog 统一 SubclassDlgItem，IDOK=Accent、IDCANCEL=Secondary；派生类已通过 DDX_Control 接管的控件自动跳过，如 FileTransferProgressDlg 的取消钮）。逐对话框附加项仍待 P4。

| 对话框 | 附加项 |
|---|---|
| `CCopyProperties` | 快速粘贴文本 Edit → Surface.Elevated + Stroke.Card；缩略图预览区加 1px 描边 |
| `CDeleteClipData` | ListCtrl 报表头自绘对齐 token（列头底 Surface.Base、1px Divider）；删除按钮用红色警示描边（`#C42B1C` 描边 + Text.Primary，非填充） |
| `CMoveToGroupDlg`/`CGroupName` | 树接 token（同 5.4 降级案）；输入框规格同 5.2 |
| `CAddType` | 纯文案框，基类化即可 |
| `CFriendPromptDlg`/`CFriendDetails` | 好友列表行高 32px + hover；IP 输入 Monospace（Consolas → Cascadia Mono 回退） |
| `CGlobalClips` | 列表接 token；已有 resizer 保留 |
| `CScriptEditor` | 编辑区底 Surface.Elevated；已有 resizer 保留 |
| `CFileTransferProgressDlg` | **CAnimateCtrl + FILECOPY.AVI 废弃**，换 `CFluentProgressBar`（新控件：8px 高圆角槽 Stroke.Card + Accent 填充）。**marquee 已移除（2026-10-03，a4e6bb5）**：自绘 marquee 无法与 DWM/GDI 帧节奏稳定同步，不确定进度回退为普通 Accent 填充条；后续只有找到可行方案再恢复 |
| `CAbout` | HyperLink 换 `Accent.Text` 色 + hover 下划线（保留 CHyperLink 类，改其颜色源）；版本号 Text.Secondary |

### 5.8 编辑器窗口链与悬浮窗

- `CEditFrameWnd`/`CEditWnd`：标题栏已复用 CDittoWindow，随 5.1 自动受益；`CTabCtrlEx`（`TabCtrl.h:169` 自绘）对齐 token：tab 底 Surface.Base、选中 tab = Accent.Subtle 胶囊 + BodyStrong 字重；RulerRichEdit 工具栏位图**本期不动**（BMP 五档 DPI 重绘成本高、低频），仅在文档标记为后续可选；
- `CToolTipEx`：底 Surface.Elevated + 1px Stroke.Card + `DwmTheme::ApplyRoundedCorners`；文字 Text.Primary/Secondary 分层；
- `CQRCodeViewer`：同上；QR 白底保留（扫码对比度需要），外加 8px 白 padding 已有则不动；
- `CNoDbFrameWnd`/`CProgressWnd`：基类化 + 文案层级（标题 BodyStrong + 说明 Secondary）。

### 5.9 图标资源策略

- **管线不变**：`IDB_* PNG` + `CGdiPlusBitmapResource` + `GdiImageDrawer::LoadStdImageDPI(id96…id350)`；
- **新增图标规格**：24 网格单色描边（1.5px）、导出 16/20/24/28/32 五档（与现有一致）、绘制时 tint 到 Text.Primary/Secondary/Accent——`GdiImageDrawer` 加 `DrawTinted(...)` 重载（GDI+ `ColorMatrix` 或 `TextureBrush+MonoClip` 实现）；
- 需要新增的图标（P1）：侧栏视图图标 6 个（全部/文本/图片/文件/链接/收藏）、chips 复用之、空状态插画 1 个（64px 多色但低饱和）；
- 旧 BMP（`Toolbar.bmp`/`back.bmp`/`bitmap1.bmp`/search 按钮三态位图，`CP_Main.rc:83-119`）：主窗口内的在 P1 被 PNG 替换后**从 rc 注释掉**（文件保留）；编辑窗工具栏位图本期不动；
- 托盘图标（Ditto2.ico 系）不在本期范围。

---

## 6. 主题兼容与深浅色策略

### 6.1 加载与映射（Theme.cpp 改造）

```
Load(themeName):
  1. ReadFile → 解析 v3 旧字段 + v4 新字段（LoadElement 不区分版本，按节点名）
  2. v4 token 缺失时派生：
     Surface.Base ← MainWindowBG；Text.Primary ← ListBoxOddRowsText；Accent ← accent/ClipPastedColor…
     （完整派生表 = 3.1 映射表的逆运算，实现为 DeriveTokensFromLegacy()）
  3. LoadWindowsAccentColor() 覆盖 Accent 系
  4. 判定主题明暗（Surface.Base 亮度 < 50% → Dark）→ 决定 DWM 暗标题栏与未覆盖控件默认值
```

- **12 个现有主题文件零修改继续可用**（Classic/Nord/Selenized 系/Mono*/Terminal…）：它们只写 v3 字段，第 2 步派生补齐 v4；
- 现有"缺节点回退灰阶"问题（旧主题选中色回退浅色）在第 2 步自然解决：派生值基于该主题自己的底/文字色，不再跨主题兜底；
- v3 主题文件里的 `GroupTreeBG/GroupTreeText` 开始真正生效（喂给自绘侧栏树）。

### 6.2 默认主题与自动切换

- 新增 `Debug/Themes/Fluent Light.xml`、`Fluent Dark.xml`（完整 v4 字段集，值 = 3.1 表）。**注册表 `Theme2` 保持空默认值** —— 空即「跟随系统明暗」，`Theme.cpp` 的空名路径会按 `DarkAppWindows10Setting()` 解析为 Fluent Dark / Fluent Light。
  **不可**把默认值改成 `Fluent Light`：安装脚本从不写 `Theme2`，非空默认值会被启动时原样采用，而跟随系统的路径（`OnWinIniChange`）只在 Windows 广播变更时触发、启动时不跑 —— 暗色系统的全新安装会永远停在浅色。用户的显式选择（含第三方主题）仍写进 `Theme2`，不受影响；
- 自动切换链路（已存在，改造点加粗）：`MainFrm.cpp:855` 定时器检测 `DarkAppWindows10Setting()` 变化 → **当前主题为 Fluent Light/Dark 成对时互换，用户手选的第三方主题不自动切**（弹托盘气泡提示一次）→ `m_Theme.Load` → `RefreshThemeColors()` 广播（`QPasteWnd.cpp:8122` 现链路）→ 各窗口 `DwmTheme::ApplyDarkCaption` + 重绘；
- `Theme.cpp:74` 现有"空主题名 + 系统暗色 → DarkerDitto"逻辑废弃，改走 Fluent Dark。

### 6.3 新 UI 文案

侧栏/chips/空状态新增的用户可见字符串走 `Debug/Language/English.xml` + `Chinese Simplified.xml`（沿用上轮已验证的做法，其他语言后续社区补）；加载机制 `MultiLanguage.cpp` 不改。

---

## 7. DPI 与可达性

### 7.1 DPI

| 项 | 现状 | 动作 |
|---|---|---|
| 进程模型 | PMv2 manifest ✅ | 不动 |
| 布局缩放 | `CDPI::Scale` 102 处 ✅ | 新代码全部经 Scale，禁止裸像素常量 |
| 主窗口跨屏 | QPasteWnd 已有 `WM_DPICHANGED` 处理（上游 `QPasteWnd.cpp:322`）✅ | 无需补；本项目的重复注册已在 361562a 移除（M8 条目过时） |
| 对话框跨屏 | 无处理 | `CFluentDialog` 统一处理（4.3 第 4 步） |
| 图标 | per-DPI PNG 五档 ✅ | 新图标按五档导出 |

### 7.2 键盘与对比度

- 现有键盘模型全保留：0-9 快捷粘贴、Ctrl+`/Enter/Esc、列表方向键、搜索即打即搜；
- 新增焦点呈现：所有自绘可交互元素（胶囊/chip/侧栏项/按钮）keyboard focus 时绘 2px FocusRing（Accent.Default）；Tab 序在侧栏引入后补齐（侧栏 → chips → 列表）；
- 对比度底线：Text.Primary/Base ≥ 7:1、Text.Secondary/Base ≥ 4.5:1、Accent.Text 在 Base 上 ≥ 4.5:1（3.3 派生时自动校验）；3.1 默认值已按此核对（Light: #1A1A1A/#F3F3F3 = 15.9:1，#616161/#F3F3F3 = 5.4:1，#005FB8/#F3F3F3 = 5.6:1；Dark: #FFFFFF/#202020 = 17.1:1，#C8C8C8/#202020 = 10.9:1，#4CC2FF/#202020 = 9.7:1）；
- 不做高对比度主题（`WM_THEMECHANGED`/HC 主题）——现状也不支持，不新增范围。

---

## 8. 分阶段实施计划、验收与风险

> 每阶段 = 一个 PR（分支 `feature/fluent-p<N>-*`），CI（`.github/workflows/pull_request.yml`，Release x64 MSBuild）绿后合并。**本机无 MFC 构建环境，构建验证走 GitHub Actions**；GUI 走查由用户手工执行（本机运行 Release64 产物）。

### P0 设计系统落地（纯新增，零行为变更）

| 票 | 内容 | 验收 |
|---|---|---|
| P0.1 | `Theme.h/.cpp`：v4 token 字段 + 派生函数 + `DeriveTokensFromLegacy()` + 亮度判定；LoadDefaults 双套（Light/Dark） | CI 绿；现有 12 主题加载结果与改造前逐字段一致（写一次性 dump 工具或临时日志核对） |
| P0.2 | `Fonts.h/.cpp` 字体服务 + 回退链实测 | CI 绿；调试日志输出选定家族 |
| P0.3 | `DwmTheme.h/.cpp` 三 API + OS 探测 | CI 绿（Win10 降级路径代码评审确认） |
| P0.4 | `FluentDialog/FluentPropertyPage/FluentButton` + vcxproj/filters 登记 | CI 绿；临时在 About 框挂基类冒烟（下阶段移除临时性） |
| P0.5 | `GdiImageDrawer::DrawTinted` + `memdc` 接入 QListCtrl 擦除路径 | CI 绿；滚动无闪烁（人工） |

### P1 主窗口

| 票 | 内容 |
|---|---|
| P1.1 | 标题栏：CDittoWindow 对齐 §5.1（DWM 圆角/暗标题栏/按钮组/CaptionSize token） |
| P1.2 | 搜索行 §5.2（SymbolEdit 去 GetSysColor、focus ring、清除钮）+ UpdateFont 换 AppFonts + QPasteWnd WM_DPICHANGED |
| P1.3 | 列表 §5.3（hover/胶囊选中/角标字体/命中色/缩略图缓存/空状态）+ 硬编码色清理（M9） |
| P1.4 | `CSidebar`（含自绘分组树）+ `CChipBar` + 类型过滤 SQL（线程 COUNT 回传模式，`QListCtrl` owner-data 现有结构内做）+ 显隐持久化 |
| P1.5 | Fluent Light/Dark 主题 XML + 默认主题切换 + 自动明暗切换链路 + 语言 XML（英/简中）条目 |

### P2 Options 外壳

| 票 | 内容 |
|---|---|
| P2.1 | `COptionsSheet` 改左导航外壳（§5.6）+ 窗口 DWM 化 |
| P2.2 | 8 页换 `CFluentPropertyPage` + Adv 属性网格 SetCustomColors 对齐 |
| P2.3 | 死代码页标记（`#pragma region` 注释 + 文档登记，不删） |

### P3 高频对话框（§5.7 表逐个一票）

### P4 编辑器/弹窗/收尾

`CTabCtrlEx` 对齐、ToolTipEx/QR/NoDb/Progress 基类化 + 圆角、FileTransfer 进度条替换 AVI、About 链接色、旧 BMP 从主窗口 rc 退役。

### 每阶段 GUI 验收清单（用户人工执行）

- 矩阵：DPI 100/125/150% × 亮/暗（含系统自动切换触发）× 英文/简中
- 功能回归：复制各类内容（文本/HTML/RTF/图片/文件）→ 弹窗显示 → 搜索（命中高亮/无结果态）→ 0-9 粘贴 → 分组（建/移/树导航）→ 好友发送 → 脚本执行 → 删除（含批量）→ Options 全 8 页过一遍 → 编辑窗打开/保存
- 视觉走查：深色下无白底控件残留；焦点环可见；hover 无残影；跨屏拖动即时缩放

### 风险表

| 风险 | 概率 | 缓解 |
|---|---|---|
| Mica 与 GDI 不透明擦除冲突，画面发灰/闪烁 | 高 | 默认关闭（注册表开关 + 实验标注）；只把圆角/暗标题栏作为默认体验 |
| 运行时 WM_SETFONT 后 DLU 布局溢出（CJK 文案比 MS Shell Dlg 宽） | 中 | resizer 默认锚定 + 每对话框走查 CJK；溢出页手动 MoveWindow 微调 |
| owner-data 列表 hover 重绘引入性能回退 | 中 | hover 只 Invalidate 单行（新旧两行矩形），不整表 |
| CPropertySheet 剥 tab 迁移破坏页面切换/Apply 隐藏逻辑 | 中 | 页面对象与 DDX 不动，仅外壳；P2 单独走全页回归 |
| 12 个旧主题派生色视觉劣化 | 中 | P0.1 字段级 dump 对比 + 每主题截图走查 |
| 三方主题用户被强制换默认主题 | 低 | `Theme2` 默认为空（跟随系统明暗），已有非空设置原样保留、不被改写 |
| 新 .cpp 忘记登记 vcxproj（MSBuild 不 glob） | 高 | 每票 checklist 含 vcxproj + .filters 两项 |

---

## 9. 明确不做的事

- **不迁移 Direct2D/DirectWrite**：MFC + GDI/GDI+ 满足全部视觉目标，迁移收益不成比例；
- **不引入第三方 UI 库**（BCG/duilib/WinUI）；
- **死代码不删**：`COptionsQuickPaste`、`COptionsUtilities`、`CSelectDB`、`CDittoPopupWindow` 仅在 P2.3 标记；删除须用户逐个确认（仓库规则）；
- **不动**：剪贴数据模型与 DB schema、网络协议（Server/Client）、U3Stop/FocusHighlight/focusdll 调试组件、托盘图标、编辑器 RulerRichEdit 工具栏位图、高对比度主题支持；
- **不做通用动画系统**：仅保留现有保存动画，新增交互均为即时状态切换。

---

## 附录 A：旧主题字段 → v4 token 派生表（实现规格）

| v4 token | 缺失时派生自 |
|---|---|
| Surface.Base | MainWindowBG |
| Surface.Elevated | SearchTextBoxFocusBG，无则 Base 提亮/压暗 4% |
| Surface.RowAlt | ListBoxEvenRowsBG |
| Text.Primary | ListBoxOddRowsText |
| Text.Secondary | Text.Primary 以 62% 透明度合成于 Base |
| Text.Disabled | Text.Secondary 以 60% 合成 |
| Accent.Default | LoadWindowsAccentColor；无则 ClipPastedColor；仍无则按明暗取 #005FB8/#4CC2FF |
| Accent.Subtle / State.SelectedBG | Accent 12% over Base |
| State.Hover | 明主题 #000000 6%、暗主题 #FFFFFF 6% over 行底 |
| Stroke.Card | Base 与 Elevated 亮度中点 |
| Stroke.Divider | Stroke.Card 再向 Base 靠 25% |
| Control.* | Elevated / Hover=Base+4% / Pressed=Hover-2% |
| Indicator.Badge | ListSmallQuickPasteIndexColor，无则 Text.Secondary |

## 附录 B：新旧对照（问题 → 方案溯源）

| 问题编号（§1） | 方案位置 |
|---|---|
| M1 Small Font | §4.2 + §5.3 |
| M2 GetSysColor 白闪 | §5.2 |
| M3 树/侧栏死字段 | §5.4 + §6.1 |
| M4 无 hover/纯色选中 | §5.3 |
| M5 标题栏纯色拼接 | §5.1 |
| M6 硬编码字体/魔法数 | §4.2 + §5.2 |
| M7 无双缓冲 | §4.5 |
| M8 无 WM_DPICHANGED | §7.1 | ~~已解决~~ 条目过时：上游本就有处理器 |
| M9 硬编码色残留 | §5.3 P1.3 |
| M10 主题模型缺 token | §3 |
| D1–D4 对话框字体/布局/主题 | §4.3 + §5.6/5.7 |
| D5 新旧图标并存 | §5.9 |
| D6 HyperLink | §5.7 About 行 |
