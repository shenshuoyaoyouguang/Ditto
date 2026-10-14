# Issue tracker: GitHub

Issues and specs for this repo live as GitHub issues in the fork **`shenshuoyaoyouguang/Ditto`** (`https://github.com/shenshuoyaoyouguang/Ditto/issues`). Use the `gh` CLI for all operations, scoping to that repo (e.g. `gh issue create -R shenshuoyaoyouguang/Ditto ...`) since the clone's `origin` points at upstream `sabrogden/Ditto`.

## Conventions

- **Create an issue**: `gh issue create -R shenshuoyaoyouguang/Ditto --title "..." --body "..."`. Use a heredoc for multi-line bodies.
- **Read an issue**: `gh issue view <number> -R shenshuoyaoyouguang/Ditto --json number,title,body,labels,comments`.
- **List issues**: `gh issue list -R shenshuoyaoyouguang/Ditto --state open --json number,title,body,labels,comments --jq '[.[] | {number, title, body, labels: [.labels[].name], comments: [.comments[].body]}]'` with appropriate `--label` and `--state` filters.
- **Make an issue a sub-issue of a parent**: `gh issue create -R shenshuoyaoyouguang/Ditto --parent <parent> ...`, or `gh issue edit <parent> -R shenshuoyaoyouguang/Ditto --add-sub-issue <child>` afterwards (`gh` 2.94+). Older `gh`: `gh api --method POST repos/shenshuoyaoyouguang/Ditto/issues/<parent>/sub_issues -F sub_issue_id=<child-db-id>` (database id, as in **Blocking** below). Without sub-issues, put `Part of #<parent>` at the top of the child body.
- **Comment on an issue**: `gh issue comment <number> -R shenshuoyaoyouguang/Ditto --body "..."`
- **Apply / remove labels**: `gh issue edit <number> -R shenshuoyaoyouguang/Ditto --add-label "..."` / `--remove-label "..."`
- **Close**: `gh issue close <number> -R shenshuoyaoyouguang/Ditto --comment "..."`

## Pull requests as a triage surface

**PRs as a request surface: no.** _(Set to `yes` if this repo treats external PRs as feature requests; `/triage` reads this flag.)_

When set to `yes`, PRs run through the same labels and states as issues, using the `gh pr` equivalents:

- **Read a PR**: `gh pr view <number> --comments` and `gh pr diff <number>` for the diff.
- **List external PRs for triage**: `gh api --paginate 'repos/shenshuoyaoyouguang/Ditto/pulls?state=open' --jq '.[] | select(.author_association | IN("OWNER","MEMBER","COLLABORATOR") | not) | {number, title, author: .user.login, author_association, labels: [.labels[].name]}'`.
- **Comment / label / close**: `gh pr comment`, `gh pr edit --add-label`/`--remove-label`, `gh pr close`.

GitHub shares one number space across issues and PRs, so a bare `#42` may be either: resolve with `gh pr view 42` and fall back to `gh issue view 42 -R shenshuoyaoyouguang/Ditto`.

## When a skill says "publish to the issue tracker"

Create a GitHub issue in `shenshuoyaoyouguang/Ditto`.

## When a skill says "fetch the relevant ticket"

Read it as in **Read an issue** above.

## Wayfinding operations

Used by `/wayfinder`. The **map** is a single issue with **child** issues as tickets.

- **Label setup**: before creating the map or any child tickets, ensure all five labels exist (Bash; safe to repeat):

  ```bash
  (
    set -euo pipefail
    for label in wayfinder:map wayfinder:research wayfinder:prototype wayfinder:grilling wayfinder:task; do
      gh label create "$label" -R shenshuoyaoyouguang/Ditto --force
    done
  )
  ```

- **Map**: a single issue labelled `wayfinder:map`, holding the Notes / Decisions-so-far / Fog body. `gh issue create -R shenshuoyaoyouguang/Ditto --label wayfinder:map`.
- **Child ticket**: an issue linked to the map as a GitHub sub-issue (see **Make an issue a sub-issue of a parent**). Where sub-issues aren't enabled, add the child to a task list in the map body and put `Part of #<map>` at the top of the child body. Labels: `wayfinder:<type>` (`research`/`prototype`/`grilling`/`task`). Once claimed, the ticket is assigned to the driving dev.
- **Blocking**: GitHub's **native issue dependencies**, the canonical, UI-visible representation. Add an edge with `gh api --method POST repos/shenshuoyaoyouguang/Ditto/issues/<child>/dependencies/blocked_by -F issue_id=<blocker-db-id>`, where `<blocker-db-id>` is the blocker's numeric **database id** (`gh api repos/shenshuoyaoyouguang/Ditto/issues/<n> --jq .id`, _not_ the `#number` or `node_id`). List blockers via the paginated `dependencies/blocked_by` endpoint and check each blocker's `state`. Where dependencies aren't available, fall back to a `Blocked by: #<n>, #<n>` line at the top of the child body. A ticket is unblocked when every blocker is closed.
- **Frontier query**: enumerate only the map's sub-issues and their assignees, then fetch every open child's blockers. The Bash query below requires `jq`, paginates both lists, and returns the first open, unassigned child with no open blockers in map order (or `null`). Replace `123` with the map's issue number:

  ```bash
  (
    set -euo pipefail
    map_number=123
    children=$(gh api --paginate "repos/shenshuoyaoyouguang/Ditto/issues/$map_number/sub_issues?per_page=100" |
      jq -c '.[] | select(.state == "open") | {number, assignees: [.assignees[].login]}')
    while IFS= read -r child; do
      [[ -n "$child" ]] || continue
      number=$(jq -r '.number' <<< "$child")
      blockers=$(gh api --paginate "repos/shenshuoyaoyouguang/Ditto/issues/$number/dependencies/blocked_by?per_page=100" |
        jq -s '[.[][] | select(.state == "open") | .number]')
      jq -c --argjson blockers "$blockers" '. + {open_blockers: $blockers}' <<< "$child"
    done <<< "$children" |
      jq -s '[.[] | select((.assignees | length) == 0 and (.open_blockers | length) == 0)] | .[0] // null'
  )
  ```

  For the task-list fallback, read the map with **Read an issue**, enumerate only its linked child tickets in task-list order, and fetch each with `gh issue view <child> -R shenshuoyaoyouguang/Ditto --json number,state,assignees,body`. If using `Blocked by` lines, fetch every listed blocker with `gh issue view <blocker> -R shenshuoyaoyouguang/Ditto --json number,state`; apply the same open / unassigned / no-open-blockers filter.
- **Claim**: `gh issue edit <n> -R shenshuoyaoyouguang/Ditto --add-assignee @me`, the session's first ticket write.
- **Resolve**: `gh issue comment <n> -R shenshuoyaoyouguang/Ditto --body "<answer>"`, then `gh issue close <n> -R shenshuoyaoyouguang/Ditto`, then append a context pointer (gist + link) to the map's Decisions-so-far.
