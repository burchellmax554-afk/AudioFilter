This project implements a real-time adaptive noise cancellation system on an NXP MCXN947 microcontroller using an LMS adaptive FIR filter. The objective is to recover a desired audio signal that has been corrupted by noise by continuously estimating and removing the noise component.

The system accepts two input signals: a noise-corrupted audio signal and a reference noise signal. The filter uses the reference noise input to generate an estimate of the noise present in the corrupted signal. This estimated noise is then subtracted from the noisy audio, producing a cleaner output signal. The filter coefficients are continuously updated using the LMS adaptation algorithm, allowing the system to respond to changing noise conditions in real time.

The project was developed and tested using the MCUXpresso IDE, CMSIS-DSP library support, and an external audio CODEC interface. Verification was performed through real-time signal processing tests and debugger-based analysis of filter behavior and output signals.

