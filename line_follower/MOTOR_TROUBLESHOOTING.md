# DRV8835 Motor Control Troubleshooting Guide

## Current Configuration
- **Motor A**: Phase=GP16, Enable=GP17
- **Motor B**: Phase=GP18, Enable=GP19
- **Mode**: PH/EN (Phase/Enable)

## Common Issues and Solutions

### 1. DRV8835 Mode Configuration
**CRITICAL**: The DRV8835 MODE pin determines the operating mode:
- **MODE pin connected to VCC (3.3V)**: PH/EN mode (what your code expects)
- **MODE pin connected to GND**: IN/IN mode (different control scheme)

**Check**: Verify that your DRV8835's MODE pin is connected to VCC!

### 2. Motor Power Issues
**Symptoms**: Motors don't move or move weakly
**Solutions**:
- Verify motor power supply voltage (typically 6V-12V for most TT motors)
- Check that motor power supply can provide sufficient current (>1A per motor)
- Ensure GND is common between Pico2 and motor power supply
- Check for loose connections

### 3. PWM Frequency Issues
**Previous settings**: 19.5 kHz (too high for some motors)
**New settings**: 1 kHz (more suitable for DC motors)

If motors still don't respond well:
- Try different PWM frequencies by changing `PWM_CLOCK_DIVIDER` in config.h
- For 500Hz: set `PWM_CLOCK_DIVIDER` to 250.0f
- For 2kHz: set `PWM_CLOCK_DIVIDER` to 62.5f

### 4. Motor Direction Issues
**In PH/EN mode**:
- PHASE=1, ENABLE=PWM → Forward
- PHASE=0, ENABLE=PWM → Reverse
- PHASE=X, ENABLE=0 → Stop

**If motors turn in wrong direction**:
- Swap motor wires (+ and -)
- Or modify the code to invert the PHASE signal

### 5. Electrical Connections Checklist
```
Pico2 GP16 → DRV8835 AIN1/APH
Pico2 GP17 → DRV8835 AIN2/AEN
Pico2 GP18 → DRV8835 BIN1/BPH  
Pico2 GP19 → DRV8835 BIN2/BEN
Pico2 GND  → DRV8835 GND
DRV8835 MODE → VCC (3.3V or 5V depending on your setup)
Motor Power + → DRV8835 VIN
Motor Power - → DRV8835 GND (shared with Pico2 GND)
Motor A+ → DRV8835 AOUT1
Motor A- → DRV8835 AOUT2
Motor B+ → DRV8835 BOUT1
Motor B- → DRV8835 BOUT2
```

### 6. Software Debugging Steps
1. **Run the motor tests**: The updated code includes comprehensive motor tests
2. **Check serial output**: Monitor the debug messages for PWM values and directions
3. **Verify PWM signals**: Use an oscilloscope or logic analyzer to check PWM on GP17/GP19
4. **Test individual motors**: Use the test functions to isolate issues

### 7. Common Motor Problems
- **Motors hum but don't turn**: Insufficient voltage or current
- **Motors turn opposite direction**: Wrong wiring or need to invert PHASE
- **Motors stop unexpectedly**: Overcurrent protection or power supply issues
- **Jerky movement**: PWM frequency too low or mechanical binding

### 8. Alternative: IN/IN Mode
If you can't get PH/EN mode working, you can switch to IN/IN mode:
1. Connect DRV8835 MODE pin to GND
2. Modify the code to use IN/IN control scheme

## Testing Commands
When you run the code, it will:
1. Print configuration diagnostics
2. Run comprehensive motor tests (if enabled)
3. Show PWM percentages and actual values
4. Test all motor directions and combinations

## Next Steps
1. Flash the updated code
2. Monitor serial output carefully
3. Verify motor test results
4. Check hardware connections if tests fail
5. Adjust PWM settings if needed 