function onSay(cid, words, param, channel)
	if(param == '') then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Command param required.")
		return true
	end
	
	local func, err = loadstring("local cid = " .. cid .."  " ..param)
	if func then
		local successful, err = pcall(func)
		if not successful then
			error("[onSay] Error: " .. err .. ".")
			return true
		end
	else
		error("[onSay] Error: " .. err .. ".")
		return true
	end
	
	doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "The script code has been executed.")
	return true
end