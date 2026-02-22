#!/usr/bin/env python3
"""Interactive pumping lemma game loop for a few classic non-regular languages."""

import random  # Picks pumping lengths and decompositions quickly.
import re  # Validates structured strings like a^n b^n using concise regexes.
from typing import Callable, Dict  # Gives tiny type hints for readability.


def lang_anbn(text: str) -> bool:
    """Return True exactly for strings a^n b^n with n>=1."""
    if not re.fullmatch(r"a+b+", text):  # Enforce only a-run followed by b-run.
        return False
    a_len = len(text) - len(text.lstrip("a"))  # Count the leading a-block length.
    b_len = len(text) - a_len  # Remaining characters must be b's by regex guard.
    return a_len == b_len  # Language membership means equal run sizes.


def lang_anbncn(text: str) -> bool:
    """Return True exactly for a^n b^n c^n with n>=1."""
    if not re.fullmatch(r"a+b+c+", text):  # Need blocks in order with no other chars.
        return False
    a_len = len(text) - len(text.lstrip("a"))  # Measure the a-block.
    rest = text[a_len:]  # Strip the a-block to inspect the b and c portions.
    b_len = len(rest) - len(rest.lstrip("b"))  # Size of the contiguous b-block.
    c_len = len(text) - a_len - b_len  # Remaining suffix must be c's.
    return a_len == b_len == c_len  # All three blocks must share the same size.


def lang_palindrome(text: str) -> bool:
    """Return True when the string equals its reverse."""
    return bool(text) and text == text[::-1]  # Avoid empty string to keep p<=|s|.


LANGUAGES = [
    {"name": "L = { a^n b^n }", "checker": lang_anbn, "tip": "Force y inside the a-run."},
    {"name": "L = { a^n b^n c^n }", "checker": lang_anbncn, "tip": "Make y hit two blocks."},
    {"name": "L = { palindromes }", "checker": lang_palindrome, "tip": "Break the mirror."},
]  # Minimal catalog keeps the interface predictable.


def choose_language() -> Dict[str, Callable[[str], bool]]:
    """Display the catalog and return the selected language record."""
    print("Pick a language to attack (enter number):")
    for idx, entry in enumerate(LANGUAGES, 1):
        print(f"  {idx}. {entry['name']}")  # Show compact menu.
    while True:
        choice = input("> ").strip()
        if choice.isdigit() and 1 <= int(choice) <= len(LANGUAGES):  # Validate menu index.
            return LANGUAGES[int(choice) - 1]
        print("Please enter a valid option number.")


def pick_p() -> int:
    """Choose the pumping length (default random, user may override)."""
    default = random.randint(4, 10)  # Keep p moderate so users can reason quickly.
    print(f"Auto-select pumping length p = {default}. Press Enter to accept or type another p.")
    override = input("> ").strip()
    return int(override) if override.isdigit() and int(override) > 1 else default


def prompt_string(checker: Callable[[str], bool], p_len: int) -> str:
    """Ask the user for a legal witness string of length >= p."""
    print(f"Provide s ∈ L with length at least {p_len}.")
    while True:
        candidate = input("> ").strip()
        if len(candidate) < p_len:
            print("Too short for the chosen p. Try again.")
            continue
        if not checker(candidate):
            print("String not in the language. Try again.")
            continue
        return candidate  # Valid witness acquired.


def adversary_decompose(text: str, p_len: int) -> Dict[str, str]:
    """Produce a valid xyz split with |xy|<=p and |y|>0."""
    max_x = min(p_len - 1, len(text) - 1)  # Ensure room for a non-empty y.
    x_len = random.randint(0, max_x)  # Choose x anywhere within the first p chars.
    max_y = min(p_len - x_len, len(text) - x_len)  # y cannot push |xy| past p.
    y_len = random.randint(1, max_y)  # y must be non-empty.
    x = text[:x_len]  # Prefix chunk.
    y = text[x_len : x_len + y_len]  # Pumpable middle chunk.
    z = text[x_len + y_len :]  # Remaining suffix.
    return {"x": x, "y": y, "z": z}


def pump_string(parts: Dict[str, str], i_val: int) -> str:
    """Recombine xyz with the chosen i."""
    return f"{parts['x']}{parts['y'] * i_val}{parts['z']}"  # Direct string multiplication keeps code tiny.


def choose_i() -> int:
    """Let the user decide how many times to pump y."""
    print("Pick an i value (0, 1, 2, ...).")
    while True:
        answer = input("> ").strip()
        if answer.isdigit():
            return int(answer)
        print("i must be a non-negative integer.")


def explain_result(in_lang: bool, tip: str) -> None:
    """Print success or retry feedback plus a strategy hint."""
    if in_lang:
        print("Result still lies inside L → pumping attempt failed.")
        print(f"Suggestion: {tip}")
    else:
        print("Success! Pumped string left the language, proving non-regularity for this p.")


def play_round() -> None:
    """Run one full pumping lemma attempt."""
    lang = choose_language()  # User picks their target.
    print("Claim: This language is not regular. We will try to witness a contradiction.")
    p_len = pick_p()  # Tool (or user) decides p.
    witness = prompt_string(lang["checker"], p_len)  # User supplies s.
    parts = adversary_decompose(witness, p_len)  # Adversary picks xyz.
    print("Adversary decomposition:")
    for key in ("x", "y", "z"):
        print(f"  {key} = '{parts[key]}' (length {len(parts[key])})")
    i_val = choose_i()  # User selects pumping power.
    pumped = pump_string(parts, i_val)  # Build xy^i z.
    print(f"Pumped string xy^{i_val}z = '{pumped}' (length {len(pumped)})")
    in_lang = lang["checker"](pumped)  # Membership check decides win state.
    explain_result(in_lang, lang["tip"])  # Provide outcome plus hint.


def main() -> None:
    """Game loop wrapper so players can retry quickly."""
    print("Pumping Lemma Practice Game — break the adversary's decomposition!\n")
    while True:
        play_round()  # Execute one scenario.
        again = input("Play another round? (y/N) ").strip().lower()
        if again != "y":  # Any answer other than y stops the loop.
            print("Thanks for playing. Keep pumping!")
            return


if __name__ == "__main__":  # Standard CLI hook.
    main()
