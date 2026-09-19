#!/usr/bin/env bash
# jlink_debug.sh — Read and decode SF32LB58 nxboot debug areas via JLink
#
# Usage: ./tools/jlink_debug.sh [serial_number]
#
# Debug areas in SRAM:
#   0x20001D00 — nxboot partition info (dumped by dump_nxboot_info())
#   0x20001F00 — PSRAM init debug (PSRAM_DBG)
#   0x20001F80 — PSRAM controller debug (PSRAM_DBG2)
#   0x20001FF0 — handoff flag ("PSRM" = bootloader jumped to app)
#

set -euo pipefail

JLINK_SN="${1:-601012352}"
TMPFILE=$(mktemp /tmp/jlink_debug_XXXXXX.txt)
OUTFILE=$(mktemp /tmp/jlink_out_XXXXXX.txt)

trap "rm -f $TMPFILE $OUTFILE" EXIT

cat > "$TMPFILE" << 'EOF'
si SWD
speed 4000
connect
halt
mem32 0x20001D00 24
mem32 0x20001F00 20
mem32 0x20001F80 20
mem32 0x20001FF0 4
g
exit
EOF

echo "=== SF32LB58 nxboot debug ==="
echo "Connecting to JLink S/N $JLINK_SN..."
echo ""

JLinkExe -device SF32LB58X_NAND -if SWD -speed 4000 \
    -nogui 1 -autoconnect 1 -SelectEmuBySN "$JLINK_SN" \
    -CommandFile "$TMPFILE" 2>&1 | sed 's/\x1b\[[0-9;]*m//g' > "$OUTFILE"

# Dump all mem32 lines for parsing
# Format: ADDR = W0 W1 W2 W3
parse_all() {
    grep -E "^[0-9A-F]+ = " "$OUTFILE" | sed 's/\x1b\[[0-9;]*m//g'
}

ALL_LINES=$(parse_all)

# Read word at absolute address
read_word() {
    local addr_hex
    addr_hex=$(printf '%08X' "$1")
    local base_line
    base_line=$(echo "$ALL_LINES" | grep "^${addr_hex} = " || true)
    if [ -z "$base_line" ]; then
        printf "00000000"
        return
    fi
    local offset=$(( (addr_hex - addr_hex) % 16 ))  # not needed
    # Each line has 4 words; figure out which field
    local base_addr=$((16#${addr_hex}))
    local rel=$(( (base_addr - (base_addr & ~15)) / 4 ))
    local field=$((rel + 2))
    echo "$base_line" | awk -v f=$field '{gsub(/\x1b\[[0-9;]*m/,"",$f); print $f}'
}

# Simpler: just read all the raw data and parse by position
# 0x20001D00 area: 6 lines × 4 words = 24 words
NXBOOT_LINES=$(echo "$ALL_LINES" | grep "^20001D")
PSRAM_LINES=$(echo "$ALL_LINES" | grep "^20001F0")
PSRAM2_LINES=$(echo "$ALL_LINES" | grep "^20001F8\|^20001F9\|^20001FA\|^20001FB\|^20001FC\|^20001FD")
HANDOFF_LINE=$(echo "$ALL_LINES" | grep "^20001FF0 =" | head -1 || true)

# Flatten all lines into a word array
flatten() {
    echo "$1" | awk '{for(i=2;i<=NF;i++) if($i ~ /^[0-9A-F]+$/) print $i}'
}

NXBOOT_WORDS=($(flatten "$NXBOOT_LINES"))
PSRAM_WORDS=($(flatten "$PSRAM_LINES"))
PSRAM2_WORDS=($(flatten "$PSRAM2_LINES"))

get_word() {
    local -n arr=$1
    local idx=$2
    if [ $idx -lt ${#arr[@]} ]; then
        echo "${arr[$idx]}"
    else
        echo "00000000"
    fi
}

hex_to_dec() {
    echo $((16#$1))
}

echo "=========================================="
echo " nxboot Partition Info (0x20001D00)"
echo "=========================================="
echo ""

MAGIC=$(get_word NXBOOT_WORDS 0)
PART_SIZE=$(get_word NXBOOT_WORDS 1)
ERR_BITS=$(get_word NXBOOT_WORDS 2)

if [ "$MAGIC" = "4E584254" ]; then
    echo "  Magic:           NXBT (OK)"
else
    echo "  Magic:           0x$MAGIC"
fi

ps_dec=$(hex_to_dec "$PART_SIZE")
echo "  Partition size:  $ps_dec bytes ($((ps_dec / 1024 / 1024)) MB)"
echo "  Open errors:     0x$ERR_BITS $([ "$ERR_BITS" = "00000000" ] && echo '(all OK)' || echo '(FAILED)')"

PART_NAMES=("Primary" "Secondary" "Tertiary")
for i in 0 1 2; do
    BASE=$((3 + i * 5))
    PMAGIC=$(get_word NXBOOT_WORDS $((BASE + 0)))
    PSIZE=$(get_word NXBOOT_WORDS $((BASE + 1)))
    PCRC=$(get_word NXBOOT_WORDS $((BASE + 2)))
    PVERSION=$(get_word NXBOOT_WORDS $((BASE + 3)))
    PPATCH=$(get_word NXBOOT_WORDS $((BASE + 4)))

    echo ""
    echo "  --- ${PART_NAMES[$i]} partition ---"

    case "$PMAGIC" in
        534F584E) echo "  Magic:   0x$PMAGIC (NXOS — unconfirmed)" ;;
        ACA0ABB1) echo "  Magic:   0x$PMAGIC (NXOS_INV — confirmed)" ;;
        00000000) echo "  Magic:   0x$PMAGIC (empty)" ;;
        *)        echo "  Magic:   0x$PMAGIC" ;;
    esac

    if [ "$PSIZE" != "00000000" ]; then
        sz=$(hex_to_dec "$PSIZE")
        echo "  Size:    $sz bytes ($((sz / 1024)) KB)"
    else
        echo "  Size:    0"
    fi

    echo "  CRC:     0x$PCRC"
    ver=$(hex_to_dec "$PVERSION")
    major=$(( (ver >> 16) & 0xFFFF ))
    minor=$(( ver & 0xFFFF ))
    patch=$(hex_to_dec "$PPATCH")
    echo "  Version: $major.$minor.$patch"
done

echo ""
echo "=========================================="
echo " PSRAM Init Debug (0x20001F00)"
echo "=========================================="
echo ""

for i in 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19; do
    w=$(get_word PSRAM_WORDS $i)
    if [ "$w" != "00000000" ]; then
        addr=$((0x20001F00 + i * 4))
        printf "  [0x%08X] = 0x%s" $addr "$w"
        case $i in
            7)  printf "  ← board_late_initialize() done" ;;
            11) printf "  ← bsp_psramc_init() done" ;;
        esac
        echo ""
    fi
done

echo ""
echo "=========================================="
echo " PSRAM Controller Debug (0x20001F80)"
echo "=========================================="
echo ""

for i in 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19; do
    w=$(get_word PSRAM2_WORDS $i)
    if [ "$w" != "00000000" ]; then
        addr=$((0x20001F80 + i * 4))
        printf "  [0x%08X] = 0x%s" $addr "$w"
        case $i in
            1)  printf "  ← IDR" ;;
            2)  printf "  ← ENR2 (MPI1 clock)" ;;
            3)  printf "  ← CSR (DLL)" ;;
            4)  printf "  ← CR (QSPI control)" ;;
            10) printf "  ← spin removed marker" ;;
            11) printf "  ← CR (EN+HWIFE)" ;;
            12) printf "  ← psram[0] at 0x60000000" ;;
            15) printf "  ← psram[0] after write" ;;
        esac
        echo ""
    fi
done

echo ""
echo "=========================================="
echo " Handoff Flag (0x20001FF0)"
echo "=========================================="
echo ""

HANDOFF_VAL=$(echo "$HANDOFF_LINE" | awk '{print $3}' | tr -d '[:space:]')
if [ "$HANDOFF_VAL" = "5053524D" ]; then
    echo "  Magic: PSRM -- bootloader handed off to app"
elif [ -z "$HANDOFF_VAL" ] || [ "$HANDOFF_VAL" = "00000000" ]; then
    echo "  Not set (bootloader still running or first boot)"
else
    echo "  Magic: 0x$HANDOFF_VAL"
fi
echo ""

# Show PC from halt output
PC_LINE=$(grep "^PC = " "$OUTFILE" | head -1 | sed 's/\x1b\[[0-9;]*m//g')
if [ -n "$PC_LINE" ]; then
    echo "=========================================="
    echo " CPU State"
    echo "=========================================="
    echo "  $PC_LINE"
    PC_HEX=$(echo "$PC_LINE" | awk '{print $3}' | tr -d ',')
    pc_val=$((16#$PC_HEX))
    if [ $pc_val -ge $((16#60000000)) ] && [ $pc_val -lt $((16#61000000)) ]; then
        echo "  -> Running from PSRAM (application is active)"
    elif [ $pc_val -ge $((16#1C000000)) ] && [ $pc_val -lt $((16#1D000000)) ]; then
        echo "  -> Running from flash (bootloader)"
    fi
fi
echo ""
