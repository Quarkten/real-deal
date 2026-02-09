PROGRAM:NGROKSET
:ClrHome
:Disp "Ngrok URL Setup"
:Disp "v1.0"
:Disp ""
:Disp "Current URL:"
:Send(18,C)
:GetCalc(Str0)
:Disp Str0
:Disp ""
:Disp "Enter new URL:"
:Input "URL:",Str1
:ClrHome
:Disp "Updating..."
:Disp "Please wait"
:Disp ""
:Send(19,C)
:Send(Str1)
:GetCalc(S)
:If S=1
:Then
:ClrHome
:Disp "Updated!"
:Disp ""
:Disp "New URL:"
:Disp Str1
:Disp ""
:Disp "Changes take effect"
:Disp "immediately"
:Pause
:Else
:ClrHome
:Disp "Update Failed"
:Disp ""
:Disp "Error code:"
:Disp S
:Disp ""
:Disp "Check URL format"
:Disp "Must contain 'ngrok'"
:Pause
:End
:ClrHome
:Disp "Ngrok Setup"
:Disp "Complete"
:Stop
