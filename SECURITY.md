# Security Policy

## Overview

Axiom is a local-first application. It does not operate a server, does not transmit your data to any Axiom-controlled infrastructure, and does not collect telemetry. All data — market data, configuration, API keys, cached bars — lives on your machine.

This document describes how API keys are stored, the threat model you should be aware of, and how to report a vulnerability.

---

## API Key Storage

Axiom stores API keys in a plaintext configuration file at:

```
~/.config/axiom/config
```

This file is created with permissions `600` (readable only by the owning user) on first write. It is never transmitted anywhere by Axiom itself.

### Threat Model

| Threat | Axiom's posture |
|---|---|
| Network interception of your API keys | Not applicable — keys never leave your machine via Axiom |
| Another process on your machine reading the config file | Mitigated by `600` permissions; not guaranteed if running as root |
| Accidental commit of the config file to a public repository | **You** must ensure `~/.config/axiom/` is in your global `.gitignore` |
| A malicious Axiom build reading your keys | Verify the binary you run — always build from source or from a signed release |

Axiom does **not** encrypt keys at rest. If your threat model requires encrypted secrets storage (e.g. you are running Axiom on a shared or multi-user machine), use the environment variable method described below instead.

---

## Environment Variable Fallback

As an alternative to storing keys in the config file, Axiom reads the following environment variables at startup. If set, they take precedence over the config file value.

| Variable | Data source |
|---|---|
| `AXIOM_API_KEY_FINNHUB` | Finnhub |
| `AXIOM_API_KEY_YAHOO` | Yahoo Finance (if applicable) |
| `AXIOM_API_KEY_POLYGON` | Polygon.io |

**Example — setting keys for a session:**
```bash
export AXIOM_API_KEY_POLYGON="your_polygon_key"
./build/axiom analyze AAPL
```

**Example — setting keys permanently via shell profile:**
```bash
# Add to ~/.bashrc or ~/.zshrc
export AXIOM_API_KEY_POLYGON="your_polygon_key"
```

**Example — using a secrets manager:**
```bash
# Pass keys inline without persisting to shell history
AXIOM_API_KEY_POLYGON=$(secret-tool lookup service polygon) ./build/axiom analyze AAPL
```

When using environment variables, the key is never written to disk by Axiom. It exists only in the process environment for the duration of the session.

---

## If You Suspect a Key Leak

If you believe your API key has been exposed:

1. **Revoke the key immediately** in your data provider's dashboard.
2. **Delete the Axiom config file** and reconfigure with a new key:
   ```bash
   rm ~/.config/axiom/config
   ./build/axiom config set POLYGON_API_KEY your_new_key
   ```
3. **Audit your shell history** — if you ever passed a key as a command-line argument (rather than via `config set` or an environment variable), it may be stored in `~/.bash_history` or `~/.zsh_history`. Clear or edit the relevant entries.
4. **Check for accidental commits** — search your git history with:
   ```bash
   git log --all --full-history -S "your_key_string"
   ```
   If found, treat the key as permanently compromised and revoke it regardless of whether the repository is private.

---

## Reporting a Vulnerability

If you discover a security vulnerability in Axiom — for example, a path that causes keys to be logged, transmitted, or written to an unprotected location — please report it responsibly.

**Do not open a public GitHub issue for security vulnerabilities.**

Contact: [security@axiom-terminal.com] *(placeholder)*

Include:
- A description of the vulnerability
- Steps to reproduce
- The version or commit hash you tested against
- Your assessment of severity and exploitability

You will receive an acknowledgement within 72 hours. We will work with you to understand the issue, develop a fix, and coordinate disclosure.

---

## Scope

This policy covers the Axiom binary and its direct dependencies as vendored or documented. It does not cover:

- Vulnerabilities in third-party data providers
- Issues with the user's operating system or shell configuration
- Vulnerabilities introduced by unofficial builds or forks
