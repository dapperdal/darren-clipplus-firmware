/* Deliberately inert layout-test entry. It performs no hardware init or
 * application handoff, so a linked artifact cannot be mistaken for alpha. */
.section .init.text, "ax", %progbits
.global clipplus_target_entry
.type clipplus_target_entry, %function
clipplus_target_entry:
    b clipplus_target_entry
.size clipplus_target_entry, .-clipplus_target_entry
