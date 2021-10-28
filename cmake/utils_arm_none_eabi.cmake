# Prints the section sizes
function(print_section_sizes TARGET)
    # Build Events command
    add_custom_command(
        TARGET ${TARGET} POST_BUILD 
        COMMAND ${CMAKE_SIZE} ${TARGET})
endfunction()

# Creates output in hex format
function(create_hex_output TARGET)
    add_custom_target(
        ${TARGET}.hex ALL
        DEPENDS ${TARGET} 
        COMMAND ${CMAKE_OBJCOPY} --output-target ihex ${TARGET} ${TARGET}.hex)
endfunction()

# Creates output in binary format
function(create_bin_output TARGET)
    add_custom_target(
        ${TARGET}.bin ALL
        DEPENDS ${TARGET} 
        COMMAND ${CMAKE_OBJCOPY} --output-target binary --strip-all ${TARGET} ${TARGET}.bin)
endfunction()

