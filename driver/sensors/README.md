# Sensors Layer

This folder contains sensor drivers grouped by chip.

Each subfolder is independent and owns chip-specific protocol/register logic,
while the board-level transport (I2C/SPI primitives) is provided by `driver/board/`.
