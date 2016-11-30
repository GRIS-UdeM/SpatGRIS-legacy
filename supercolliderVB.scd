// cmd-enter to run line (or selection)

var numChannels = 16;

Server.local.options.numOutputBusChannels = numChannels;
s.boot;

// ~configVBAP = VBAPSpeakerArray.new(2, [ -22.5, 22.5, 67.5, 112.5, 157.5, -157.5, -112.5, -67.5 ]);

~configVBAP = VBAPSpeakerArray.new(3, [ [-22.5, 0], [22.5, 0], [-67.5, 0], [67.5, 0], [-112.5, 0], [112.5, 0], [-157.5, 0], [157.5, 0], [-60, 45], [60, 45], [-120, 45], [120, 45], [0, 45], [-180, 45], [-90, 85], [90, 85] ]);

~configVBAPBuffers = ~configVBAP.loadToBuffer;


x = {
	var source;
	var azim;
	var elev;
	var spread;


	//ar is for make audio at a fixed rate
	// source = SawOsc.ar(300, mul:0.1);
		source = LFSaw.ar(50, mul:0.1);
	// source = SawOsc.ar(MouseY.kr(100,500));

	azim = LFSaw.kr(0.1, 0).range(-180, 180) * -1;
	elev = 0.0;
	// elev = 45.0;
	// elev = 80.0;
	spread = 0;

	//arguments are numChans, in, bufnum, azimuth = 0.0, elevation = 1.0, spread = 0.0;
	VBAP.ar(numChannels, source, ~configVBAPBuffers.bufnum, azim, elev, spread);

}.play;
