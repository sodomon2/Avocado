"use strict";
$(function() {
    var ws;
    var autoReconnect = true;
    var data = {
        connecting: false,
        connected: false,
        error: false,
        cpu: {},
        event: {
            emu: {
                pause: function() {
                    console.log("Pause emulation");
                    ws.send("P");
                },
                step: function() {
                    console.log("Single step");
                    ws.send("S");
                }
            },
            connect: function() {
                connect();
                autoReconnect = true;
            },
            disconnect: function() {
                ws.close();
                autoReconnect = false;
            }
        }
    };

    function hex(value, size) {
        if (value == undefined) {
            return "";
        }
        var h = value.toString(16);
        while (h.length < size) {
            h = "0" + h;
        }
        return "0x" + h;
    }

    rivets.formatters.hex32 = function(value) {
        return hex(value, 8);
    }
    rivets.formatters.hex16 = function(value) {
        return hex(value, 4);
    }

    rivets.bind($('#info'), data);
    connect();

    function connect() {
        ws = new WebSocket("ws://liquid.noip.me:3000");
        var interval = 0;
        
        ws.onopen = function(evt) { 
            data.connected = true;
            data.error = false;
            interval = setInterval(function() {
                ws.send(".");
            }, 100);
        };
        ws.onclose = function(evt) {
            data.connected = false;
            data.cpu = {};
            clearInterval(interval);

            if (autoReconnect) {
                connect();
            }
        };
        ws.onmessage = function(evt) { 
            var j = JSON.parse(evt.data);
            data.emu = j;
        }
        ws.onerror = function(evt) { 
            data.connected = false;
            data.error = true;
            clearInterval(interval);
        };
    }
});