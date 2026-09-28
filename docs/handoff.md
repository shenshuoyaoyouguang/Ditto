# Ditto `src/` 反冗余清理 — 交接文档

## 元信息

| 项 | 值 |
|---|---|
| 生成日期 | 2026-09-29 |
| 生成者 | CodeArts（AI 代码智能体） |
| 交接对象 | 团队同事（熟 C++/MFC 且了解 Ditto 代码库） |
| 关联 PR | [shenshuoyaoyouguang/Ditto#3](https://github.com/shenshuoyaoyouguang/Ditto/pull/3) |
| 关联分支 | `deslop/src-cleanup` |
| 验证方式 | GitHub Actions CI（MSBuild Release x64） |

> **行号说明**：下文剩余任务的行号基于**清理前**的原始代码快照。PR #3 已删除约 460 行，合并后行号整体前移。执行任何剩余项前，请在合并 PR #3 后的代码上重新 `grep`/`rg` 定位。

## 1. 任务背景

对 Ditto 主程序源码 `src/`（287 文件）做有界"反冗余"清理：删除死代码、未使用变量、注释残留、重复逻辑与零引用符号，**不改变运行时行为**。本轮已完成全部零风险删除并通过 CI；中/高风险重构项与行为修复项留作后续。

## 2. 当前进度

### 已完成（PR #3，CI ✓）

- **规模**：30 文件，净删 460 行（+32/−492）
- **CI**：`Build 64bit` 通过（4m28s，`Restore NuGet packages` ✓ → `Build 64bit` ✓）
- **引用复核**：全仓 `grep` 确认所有已删符号零残留引用

### 已完成分类

- **Pass 1（注释/调试残留）**：`QPasteWnd`、`Options`、`DatabaseUtilities`、`OptionsGeneral`、`SendKeys`、`ChaiScriptOnCopy`、`MainFrm`、`OleClipSource`、`Clip`、`Misc`、`Theme`、`WndEx`、`QListCtrl`
- **Pass 2（未使用变量/重复逻辑）**：`Clip`、`DeleteClipData`、`Client`/`Server`、`EditWnd`、`ChaiScriptXml`、`ImageViewer`、`Slugify`、`Client.h`、`Server.h`、`OptionsSheet`、`QuickPaste`、`QuickPasteKeyboard`
- **Pass 3（零引用死符号）**：`SetSearchImages`、`DrawWindowIcon`、`CreateBackup`、`GetOLDDefaultDBName`

## 3. 剩余任务

按优先级与风险分四档。**每项建议**：独立分支 → 小步提交 → 推 fork → CI 验证 → 手动冒烟 → 合入。

### P0 — 安全隐患（建议最先处理）

| # | 任务 | 文件:行（清理前） | 说明 | 风险 |
|---|---|---|---|---|
| 1 | 删除崩溃后门 | `QPasteWnd.cpp` `OnNMClickList1` 内 `raise(SIGSEGV)`（约 7316 行） | 搜索框输入 `crash` 且 Ctrl+Shift 时故意触发段错误，开发期后门 | 低（独立条件块，删除不影响正常点击逻辑） |

**验证**：CI + 手动确认搜索框点击/双击/右键正常。

### P1 — 低风险清理（与已完成项同类，零行为变更）

| # | 任务 | 文件:行（清理前） | 说明 | 风险 |
|---|---|---|---|---|
| 2 | 删裸 `OutputDebugString` | `QPasteWnd.cpp` `OnTimer` 约 6569、6587 行 | 发布版也会执行；建议删除或包 `#ifdef _DEBUG` | 低 |
| 3 | 删未使用局部变量 | `CP_Main.cpp` 约 302-366（`ret`）、`SimpleBrowser.cpp` 多处（`result`）、`ProcessPaste.cpp:181`（`clipId`）、`Misc.cpp:265`（`bResult`） | 赋值后从未读取；保留其 `SendMessage`/`EnumWindows` 副作用调用，仅去变量 | 低 |
| 4 | 删恒真分支 | `HotKeys.cpp:409` `if(pKey != NULL)` | 前文 402-405 已保证非空 | 低 |
| 5 | 删死分支 | `ClipEditThread.cpp:203-206` `if(pNotify == nullptr)` | 指针运算 + `NextEntryOffset>0` 保证非空 | 低 |
| 6 | 删死函数链 | `Misc.cpp` `MyMonitorEnumProc`+`MONITOR_ENUM_PARAM`+`MONITOR_SEARCH_METOHD`（554-615）、`RemoveEscapes`/`GetEscapeChar`（189/209）、`IsRunningLimited`（1090）；`Misc.h:93-94,119,184` | 全仓零引用（已 grep 确证）；注意保留 `IsVista`（仍被 `ClipboardViewer` 使用） | 低 |
| 7 | 删死函数 | `ClipIds.cpp` `LoadElementsOf`（233）、`CopyTo`（260）；`ClipIds.h:29,31` | 全仓零调用 | 低 |
| 8 | 删封装层死代码 | `CppSQLite3.cpp` `sqlite3_encode/decode_binary`（1148-1220）、`tableExists`/`execScalarEx`/`getBlobFieldSize`/`fieldDeclType`/`interrupt`；对应 `.h` 声明 | 无调用点 | 低-中（public API，确认无 add-in/外部链接依赖后再删） |

### P2 — 中/高风险重构（需 CI + 手动回归）

| # | 任务 | 文件:行（清理前） | 说明 | 风险 |
|---|---|---|---|---|
| 9 | 合并 `DoPaste*` 14 函数 | `QPasteWnd.cpp:4925-5115`、`QPasteWnd.h:286-299` | 14 个同构函数（仅 `pasteOptions.m_xxx=true` 字段不同），可参数化为单一 helper；调用点在 `DoAction` switch（3292-3319、3556-3565） | 中 |
| 10 | 去重 `HslToRgb` | `QListCtrl.cpp:676`（类成员）vs `Theme.cpp:225`（全局） | 两份近乎相同实现；注意签名 `float` vs `double` | 中 |
| 11 | 合并 `MoveUp`/`MoveDown` | `Clip.cpp:1095-1294` | 约 200 行对称复制，仅 `>`/`<`、`ASC`/`DESC`、`+1`/`-1` 相反 | 高（排序算法，需回归） |
| 12 | 删整类 `CSystemTray` | `SystemTray.cpp/.h`（1133 行）+ `CP_Main.vcxproj:750,956` + `MainFrm.h:3,61` 注释 | 已被 `CTrayNotifyIcon` 取代；需同步改工程文件 | 中 |
| 13 | 删整类 `COptionsQuickPaste`/`COptionsUtilities` | 对应 `.cpp/.h` + `CP_Main.vcxproj` | 废弃属性页，`OptionsSheet` 已注释其装配；需确认 `IDD_*` 资源 ID 无复用 | 中 |
| 14 | 删零调用 setter 群 | `Options.cpp:2486-2882`、`Options.h:521-624` 约 20 个（如 `SetGroupDoubleClickTimeMS`、`SetSaveToGroupTimeoutMS` 等） | 设置 UI 已迁移到 `AdvGeneral` 属性网格 | 中（确认无外部链接） |
| 15 | 删库式 getter 群 | `RichEditCtrlEx.cpp:244-524` 约 18 个、`HyperLink.cpp:187-273` 约 10 个 | CodeProject 控件库残留，全仓外部引用为 0 | 中（public API） |
| 16 | 清理 alt-image 机制 | `GdipButton.cpp` `LoadAltImage`（205）+ `EnableToggle`/`SetImage`（460/475）+ `CtlColor`/`DrawItem` 内 6 处 `m_bHaveAltImage` 分支 | 无入口的完整闭环 | 中 |
| 17 | 合并 `OnMouseWheel`/`OnMouseHWheel` | `ScrollHelper.cpp:329-415` | 近乎逐行相同；顺带修 `OnMouseHWheel` 误用 `SB_VERT` 的横向滚轮 bug | 中 |

### P3 — 行为修复（需产品确认，非纯清理）

| # | 任务 | 文件:行 | 说明 |
|---|---|---|---|
| 18 | 修 getter 用错 | `AdvGeneral.cpp:339-340` | `SETTING_DEBUG_TO_OUTPUT_STRING` 误用 `GetEnableDebugLogging()`，应为 `GetEnableOutputDebugStringLogging()`（对应 setter 分支已正确） |
| 19 | 修 `Format` 缺占位符 | `OptionsQuickPaste.cpp:314` | 格式串仅 1 个 `%s` 却传 2 参数，`theme.LastError()` 被忽略 |

## 4. 依赖项

- **所有任务**：MFC 构建环境（本机缺，CI 已验证可用）
- **P2 #12/#13**：`CP_Main.vcxproj` 与 `.filters` 须同步修改（MSBuild 不 glob）
- **P2 #11**：排序行为回归（项目无测试套件，需手动冒烟：移动 clip 上下/置顶/分组）
- **P2 #17**：横向滚轮行为确认
- **P3**：产品负责人确认预期行为

## 5. 风险评估

| 档 | 风险 | 缓解 |
|---|---|---|
| 已完成 | 零行为风险 | CI 已证编译通过；纯删除/去重 |
| P0 | 删除后门属行为变更（特定输入不再崩溃），无业务影响 | CI + 手动 |
| P1 | 零行为风险，与已完成同类 | CI |
| P2 | 重构可能引入逻辑差异 | 每项独立 CI + 手动冒烟；小步提交便于二分回退 |
| P3 | 改变运行时行为 | 产品确认预期 |

**全局风险**：项目无单元测试，回归依赖手动冒烟（剪贴板复制/粘贴、搜索、特殊粘贴、排序、网络发送、主题切换）。

## 6. 后续执行顺序

1. **P0**（崩溃后门）— 独立提交，立即处理
2. **P1**（低风险清理）— 可批量合并为一个 PR
3. **P2**（中/高风险重构）— 每项独立 PR，便于评审与回退
4. **P3**（行为修复）— 经产品确认后单独提交

完成后若希望贡献回上游，可向 `sabrogden/Ditto` 发 PR。

## 7. 验证流程（CI 路线）

本机缺 MFC，构建验证走 GitHub CI：

```bash
# fork 已存在：shenshuoyaoyouguang/Ditto
git remote add myfork git@github.com:shenshuoyaoyouguang/Ditto.git   # 若未添加
git checkout -b <task-branch>
git add src/
git commit -m "..."
git push -u myfork <task-branch>
gh pr create --repo shenshuoyaoyouguang/Ditto --base master --head <task-branch>
gh run watch <run-id> --repo shenshuoyaoyouguang/Ditto --exit-status
```

CI 配置：`.github/workflows/pull_request.yml`（`windows-latest` + `nuget restore` + `msbuild Release x64`）。

## 8. 附录：已执行项明细

详见 PR #3 提交信息（commit `bf452af`）与上文 Pass 1/2/3 分类。完整 diff：`git diff myfork/master...myfork/deslop/src-cleanup`。