const fs=require('fs');
const addon = require('./build/Release/windows-audio-capture');

let startCapture = (
    continuationObject = { continue: true },
    pollingDelayMs = 250,
    onAudioCallback = () => {}
) => {
    const c = addon.CreateCaptureClient();
    //console.log(c);
    
    addon.InitializeCom(c);
    addon.StartCapture(c);
    
    f = addon.GetAudioFormat(c);
    //console.log(f);
    formatIsValid = f[0];
    frameSize = f[1];
    numChannels = f[2];
    bitsPerSample = f[3];
    sampleRate = f[4];

    // check audio format for validity
    if(
        formatIsValid != 1 // expect formatIsValid==1 to indicate 4-byte floating point data
        || frameSize != 8 // expect 8 byte frame, 4 bytes for each of L and R channels
        || numChannels != 2 // expect 2 channels, L and R
        || bitsPerSample != 32 // expect 4 bytes = 32 bits per one audio sample
        || sampleRate != 48000 // expect 48000 sample rate
    ) {
        return false;
    } else {
        // start the polling with setInterval
        dataBuffer = new ArrayBuffer(16);
        interval = setInterval( () => {
            //console.log('JS start interval');
            if(continuationObject.continue == false) {
                console.log('JS audio capture loop finished');
                clearInterval(interval);
            } else {
                nextPacketSize = addon.GetNextPacketSize(c);
                maxPacketSize = nextPacketSize * 10;
                while(nextPacketSize > 0) {
                    //console.log('JS next packet size: ' + nextPacketSize);
                    requiredBufferSize = maxPacketSize * frameSize;
                    if(dataBuffer.byteLength < requiredBufferSize) {
                        //console.log('JS ' + dataBuffer.byteLength + " dataBuffer is too small, allocating " + requiredBufferSize);
                        dataBuffer = new ArrayBuffer(requiredBufferSize)
                    }
                    
                    nFrames = addon.GetBuffer(c, nextPacketSize, maxPacketSize, dataBuffer);
                    //console.log('JS got ' + nFrames + ' frames');
    
                    // arrayBuffer is larger than necessary to allow for possible overruns in C++ capture 
                    // side. extract the slice of the data that was actually received.
                    const receivedBufferSlice = dataBuffer.slice(0, nFrames * frameSize);
    
                    onAudioCallback(receivedBufferSlice);
    
                    nextPacketSize = addon.GetNextPacketSize(c);
                    maxPacketSize = nextPacketSize * 10;
                }
                //console.log('JS done one interval ');
            }
        }, pollingDelayMs);
    }
    return true;
}

let main = async () => {
    outputFile = 'output.bin';
    captureDuration = 10000; // ms
    console.log(`testing desktop audio capture for ${captureDuration} ms and writing to ${outputFile}`);

    try {
        fs.unlinkSync(outputFile);
    } catch(e) {
    }

    audioCallback = (dataBuffer) => {
        const uint8Array = new Uint8Array(dataBuffer);
        const buffer = Buffer.from(uint8Array);
        fs.appendFileSync(outputFile, buffer, (err) => {
          if (err) throw err; 
        });
    }

    continuationObject = { continue: true };
    // startCapture starts a timer to poll audio data and
    // continues as long as continuationObject.continue = true
    result = startCapture(
        continuationObject, // flag to continue or not
        1000, // polling interval
        audioCallback // callback with audio data in ArrayBuffer
    );
    if(result == false) {
        console.log('error starting capture');
    } else {
        // stop the capture after 5 seconds
        setTimeout( () => {
            continuationObject.continue = false;
        }, 10000);

        // debug message after capture stops to ensure we did not crash on C++ side
        setTimeout( () => {
            console.log(`JS audio capture finished. raw output written to ${outputFile}.`);
            console.log('convert to wav file with following command:');
            console.log(`ffmpeg.exe -f f32le -ac 2 -ar 48000 -i ${outputFile} output.wav`)
        }, 11000)
    }
}

main();
