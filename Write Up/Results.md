The adaptive LMS noise cancellation system was successfully implemented and tested on the MCXN947 MCU. Input consisted of a noise-corrupted audio signal and a reference noise signal.
The LMS FIR filter continuously adapted its coefficients to estimate the noise component present in the corrupted signal. The estimated noise signal was subtracted from the noisy 
input, producing an error signal that represented the recovered audio output. Testing showed that the adaptive filter converged and reduced the correlated noise component while 
preserving the desired signal content. The successful operation of the system was verified through debugger observations and output signal analysis. The results demonstrate that the 
LMS adaptive filtering process effectively performs real-time noise cancellation and can recover a cleaner version of the original signal.
