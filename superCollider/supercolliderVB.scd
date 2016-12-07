(
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
)



//================= FROM THE EXAMPLE  HERE http://doc.sccode.org/Classes/VBAP.html =====================

// Server.default = s = Server.internal;
// // 2D
// a = VBAPSpeakerArray.new(2, [ -22.5, 22.5, 67.5, 112.5, 157.5, -157.5, -112.5, -67.5 ]); // 8 channel ring
//
// a.speakers[1].dump;
//
// b = a.loadToBuffer;
//
// (
// x = { |azi = 0, ele = 0, spr = 0|
// 	VBAP.ar(8, PinkNoise.ar(0.2), b.bufnum, azi, ele, spr);
// }.scope;
// )
//
// // test them out
// {2, [ -22.5, 22.5, 67.5, 112.5, 157.5, -157.5, -112.5, -67.5 ].do({|ang| x.set(\azi, ang); 1.wait; }) }.fork;
//
// // try the spread
// x.set(\spr, 20);
// x.set(\spr, 100); // all speakers
//
// x.free; b.free;
//
// // 3D
// a = VBAPSpeakerArray.new(3, [[-22.5, 14.97], [22.5, 14.97], [-67.5, 14.97], [67.5, 14.97], [-112.5, 14.97], [112.5, 14.97], [-157.5, 14.97], [157.5, 14.97], [-45, 0], [45, 0], [-90, 0], [90, 0], [-135, 0], [135, 0], [0, 0], [180, 0]]); // zig zag partial dome
//
// b = Buffer.loadCollection(s, a.getSetsAndMatrices);
//
// (
// // pan around the circle up and down
// x = { |azi = 0, ele = 0, spr = 0|
// 	var source;
// 	source = PinkNoise.ar(0.2);
// 	VBAP.ar(16, source, b.bufnum, LFSaw.kr(0.5, 0).range(-180, 180) * -1, SinOsc.kr(3, 0).range(0, 14.97), spr);
// }.play;
// )

//////// 5.1 GUI example with CircleRamp

(
var speakerList, x=200, y=150, targx=200, targy=150;
var atorad = (2pi) / 360, rtoang = 360.0 / (2pi);
var targRotate, actRotate, targPoint, actPoint;
var maxShiftPerFrame = 20, frameInterval = 0.01;
var resched = false, count = 0;
var panBus, widthBus, recButton;
var a, b, c, e;

maxShiftPerFrame = maxShiftPerFrame * atorad;
actPoint = Point(x, y) - Point(200, 200);
panBus = Bus.control;
widthBus = Bus.control.set(60);

w = Window.new("5.1 Panner", Rect(128, 64, 400, 450)).front;
w.view.background_(Color.grey(0.3));
w.view.decorator = FlowLayout(w.view.bounds);
speakerList = [[-30, "L"], [30, "R"], [0, "C"], [-110, "Ls"], [110, "Rs"]];
c = UserView.new(w,Rect(0, 0, 400, 380));
c.canFocus = false;

c.drawFunc = {
    Color.grey(0.8).set;
    // draw the speaker layout
    Pen.translate(200,200);
    ((actPoint.theta + (0.5pi)).wrap2(pi) * rtoang).round(0.01).asString.drawCenteredIn(Rect.aboutPoint(0@170, 30, 10), Font.new("Arial", 10), Color.grey(0.8));
    Pen.strokeOval(Rect.aboutPoint(0@0, 150, 150));
    Pen.rotate(pi);
    speakerList.do({|spkr|
        Pen.use({
            Pen.rotate(spkr[0] * atorad);
            Pen.moveTo(0@170);
            Pen.strokeRect(r = Rect.aboutPoint(0@170, 30, 10));
            if(spkr[0].abs < 90, {
                Pen.use({
                    Pen.translate(0, 170);
                    Pen.rotate(pi);
                    spkr[1].drawCenteredIn(Rect.aboutPoint(0@0, 30, 10),
                        GUI.font.new("Arial", 10), Color.grey(0.8));
                });
            },{
                spkr[1].drawCenteredIn(r, GUI.font.new("Arial", 10), Color.grey(0.8));
            });
        });
    });

    Pen.moveTo(0@0);

    // draw the pan point
    Pen.rotate(actPoint.theta + 0.5pi);
    targPoint = Point(x, y) - Point(200, 200);
    // trunc to avoid loops due to fp math
    targRotate = (targPoint.theta - actPoint.theta).trunc(1e-15);
    // wrap around
    if(targRotate.abs > pi, {targRotate = (2pi - targRotate.abs) * targRotate.sign.neg});
    actRotate = targRotate.clip2(maxShiftPerFrame).trunc(1e-15);
    actPoint = actPoint.rotate(actRotate);
    Pen.rotate(actRotate);
    Pen.lineTo(0@150);
    Pen.stroke;
    Pen.fillOval(Rect.aboutPoint(0@150, 7, 7));
    Pen.addWedge(0@0, 140, neg(e.value * 0.5) * atorad + 0.5pi, e.value * atorad);
    Pen.stroke;
    Color.grey(0.8).alpha_(0.1).set;
    Pen.addWedge(0@0, 140, neg(e.value * 0.5) * atorad + 0.5pi, e.value * atorad);
    Pen.fill;

    if((actRotate.abs > 0), {AppClock.sched(frameInterval, {w.refresh})}, {count = 0;});
    if(count%4 == 0, {panBus.set((actPoint.theta + (0.5pi)).wrap2(pi) * rtoang)});
};
c.mouseMoveAction_({|v,inx,iny| x = inx; y = iny; w.refresh;});
c.mouseDownAction_({|v,inx,iny| x = inx; y = iny; w.refresh;});
e = EZSlider.new(w, 380@20, "Stereo Width", [0, 180].asSpec, {arg ez; widthBus.set(ez.value); w.refresh}, labelWidth: 80);
e.labelView.setProperty(\stringColor,Color.grey(0.8));

w.refresh;

// VBAP

a = VBAPSpeakerArray.new(2, speakerList.collect(_.first));
b = a.loadToBuffer;

SynthDef('VBAP 5 chan', { |azi = 0, ele = 0, spr = 0, width = 60, vbapBuf|
var panned, source;
source = SinOsc.ar([440, 660], 0, Decay2.ar(Impulse.ar([1, 0.9]), 0.1, 0.2));
azi = azi.circleRamp;
panned = VBAP.ar(5, source, vbapBuf, [azi - (0.5 * width), azi + (0.5 * width)], ele, spr);
// 'standard' channel order for 5.1
[0, 1, 2, 4, 5].do({arg bus, i; Out.ar(bus, panned[0][i])});
[0, 1, 2, 4, 5].do({arg bus, i; Out.ar(bus, panned[1][i])});

}).play(s, [vbapBuf: b.bufnum, azi: panBus.asMap, width: widthBus.asMap]);

)