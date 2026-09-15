# XRobot maintenance and review

XRobot is a community project. LibXR is its independently usable kernel;
XRobot tooling, CodeGenerator and reusable packages form the surrounding
source ecosystem. Each organization may own its own Modules and BSPs.

The Project Owner is **Jiu-xiao** and retains administrative/merge override and
final responsibility for architecture, maintainer appointment and disputes.
Core maintainers own cross-component contracts and releases. Subsystem
maintainers review their backend or area; package maintainers own individual
Modules/BSPs. Existing contributors are not automatically appointed to a role.
Until further maintainers are explicitly appointed, CODEOWNERS falls back to
the Project Owner rather than inventing a team or reviewer.

Normal core and official Module work targets `dev`. Stable promotion targets
`master` after the relevant integration acceptance. Third-party packages may use
their own branch model. Backend changes must meet the same architecture and
peripheral-test standard regardless of author. Cross-core API changes require
core review in addition to the affected subsystem review.

`community`, `verified`, and `official` describe distinct maintenance/validation
claims. Official status means an accepted ongoing maintenance responsibility,
not merely an organization namespace. Hardware verification is tied to a
specific package ref and LibXR baseline. Maintainers decide whether a change
invalidates earlier evidence; compilation alone does not grant hardware status.
Detailed evidence stays in PRs, tests and release notes, not an expanding registry
database. The current process assumes no hardware farm.

Repository transfer, branch protection, GitHub permissions, public releases and
package publication are separate administrative operations. This policy and a
CODEOWNERS file do not by themselves enable or prove any branch protection rule.
