# mod-autobalance: Sovereign Module Agent Instructions

This document governs autonomous agent operations within `mod-autobalance`.

---

## 1. Module Domain & Architecture
- **Repository**: `mod-autobalance`
- **Category**: Dynamic Dungeon & Raid Scaling System
- **Entrypoint**: `src/AutoBalance.cpp` (`AddSC_AutoBalance`)
- **Configuration**: `conf/autobalance.conf.dist`
- **Issue Tracker**: [`docs/ISSUES.md`](docs/ISSUES.md)
- **Maintainer Skill**: [`autobalance-maintainer`](../skills/autobalance-maintainer/SKILL.md)

---

## 2. Architectural Highlights & Reusable Code Patterns
`mod-autobalance` is the gold standard reference implementation for:
1. **In-Memory Stat Scaling**: Multiplies creature health, mana, and damage dynamically via `AllCreatureScript::OnCreatureDamageTaken` without altering static database templates.
2. **Player Group Size Curves**: Dynamically calculates scaling coefficients based on player count and level variance.
3. **Boss Scaling Modifiers**: Custom multipliers for dungeon bosses vs trash mobs.

---

## 3. Operating Invariants
1. **The Two Core Pillars are IMMUTABLE**: Never modify `azerothcore-wotlk` or `mod-playerbots`.
2. **Zero-Prompt Auto Mode**: Follow the autonomous execution playbook in [`docs/framework/10_AUTONOMOUS_AGENT_REPAIR_AND_PORTING_PLAYBOOK.md`](../docs/framework/10_AUTONOMOUS_AGENT_REPAIR_AND_PORTING_PLAYBOOK.md).
3. **Idempotency & Thread Safety**: All in-memory lookups must be guarded with mutex locks.
