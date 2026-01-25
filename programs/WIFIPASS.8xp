PROGRAM:WIFIPASS
:ClrHome
:Disp "WiFi Connect"
:Disp "v1.0"
:Disp ""
:Disp "Enter SSID:"
:Input "SSID:",Str1
:Disp ""
:Disp "Enter Password:"
:Input "Password:",Str2
:ClrHome
:Disp "Connecting..."
:Disp "Please wait"
:Disp ""
:Send(16,C)
:Send(Str1)
:Send(Str2)
:GetCalc(S)
:If S=1
:Then
:ClrHome
:Disp "Connected!"
:Disp ""
:Disp "WiFi Status:"
:Disp "Active"
:Disp ""
:Disp "You can now:"
:Disp "- Use GPT commands"
:Disp "- Update firmware"
:Disp "- Change settings"
:Pause
:Else
:ClrHome
:Disp "Connection Failed"
:Disp ""
:Disp "Error code:"
:Disp S
:Disp ""
:Disp "Try again or"
:Disp "check password"
:Pause
:End
:ClrHome
:Disp "WiFi Setup"
:Disp "Complete"
:Stop
