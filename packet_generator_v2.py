def build_esp_packet(dest_id, send_id, packet_id, payload_data=[]):
    sof, eof = 0xAA, 0xAB
    di = 0xD0 + (dest_id & 0x0F)
    oi = 0xE0 + (send_id & 0x0F)
    pi = packet_id & 0xFF
    pl = len(payload_data) + 1
    
    packet_pre_cs = [sof, di, oi, pi, pl] + payload_data
    checksum = sum(packet_pre_cs) & 0xFF
    return packet_pre_cs + [checksum, eof]

def format_as_c_array(packet_list):
    return "{" + ", ".join([f"0x{b:02X}" for b in packet_list]) + "}"

def get_menu_choice(title, options):
    print(f"\n--- {title} ---")
    for i, opt in enumerate(options, 1):
        print(f"{i}. {opt}")
    print(f"{len(options)+1}. None/Skip")
    
    choice = input("Select #: ")
    if choice.isdigit() and 1 <= int(choice) <= len(options):
        return options[int(choice)-1]
    return None

def interactive_0x31():
    print("\n" + "="*25 + " ESP 0x31 GENERATOR " + "="*25)
    
    # 1. Bogey Counter
    bogey_val = input("Bogey Count (0-9 or blank): ")
    seg_map = {'0': 0x3F, '1': 0x06, '2': 0x5B, '3': 0x4F, '4': 0x66, 
               '5': 0x6D, '6': 0x7D, '7': 0x07, '8': 0x7F, '9': 0x6F}
    seg_img1 = seg_map.get(bogey_val, 0x00)

    # 2. Select Band (Skips other bands once one is chosen)
    selected_band = get_menu_choice("Select Band", ["Laser", "Ka Band", "K Band", "X Band"])
    
    # 3. Select Direction (Skips other directions once one is chosen)
    selected_dir = get_menu_choice("Select Direction", ["Front Arrow", "Side Arrow", "Rear Arrow"])

    # 4. Global Blink Setting
    blink = input("\nShould the alert indicators blink? (y/n): ").lower() == 'y'

    # Mapping Bits (Rear:7, Side:6, Front:5, X:3, K:2, Ka:1, Laser:0)
    bit_map = {
        "Rear Arrow": 0x80, "Side Arrow": 0x40, "Front Arrow": 0x20,
        "X Band": 0x08, "K Band": 0x04, "Ka Band": 0x02, "Laser": 0x01
    }

    # Construct Image 1 (All active bits)
    band_bits = 0x00
    if selected_band: band_bits |= bit_map[selected_band]
    if selected_dir:  band_bits |= bit_map[selected_dir]
    
    band_img1 = band_bits
    
    # Construct Image 2 (If blinking, active bits are 0)
    band_img2 = 0x00 if blink else band_img1
    seg_img2 = 0x00 if (blink and bogey_val) else seg_img1

    # 5. Final Details
    strength = int(input("\nSignal Strength (0-8, default 0): ") or 0)
    signal_byte = (1 << strength) - 1 if strength > 0 else 0
    
    vol = (int(input("Main Vol (0-15, default 8): ") or 8) << 4) | (int(input("Mute Vol (0-15, default 0): ") or 0) & 0x0F)

    # Payload Assembly
    payload = [seg_img1, seg_img2, signal_byte, band_img1, band_img2, 0x50, 0x00, vol]
    
    packet = build_esp_packet(0x08, 0x0A, 0x31, payload)
    print(f"\nGENERATED ARRAY:\n{format_as_c_array(packet)}")

if __name__ == "__main__":
    while True:
        interactive_0x31()
        if input("\nGenerate another? (y/n): ").lower() != 'y': break