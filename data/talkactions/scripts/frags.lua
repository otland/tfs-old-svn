local config = {
	fragTime = getConfigInfo('timeToDecreaseFrags')
}

function onSay(cid, words, param)
	local amount = getPlayerRedSkullTicks(cid)
	if(amount > 0 and config.fragTime > 0) then
		local frags = math.floor((amount / config.fragTime) + 1)
		local remainingTime = math.floor(amount - (config.fragTime * (frags - 1)))

		local hours = math.floor(((remainingTime / 1000) / 60) / 60)
		local minutes = math.floor(((remainingTime / 1000) / 60) - (hours * 60))
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "You have " .. frags .. " unjustified frag" .. (frags > 2 and "s" or "") .. ". The amount of unjustified frags will decrease after: " .. hours .. "h and " .. minutes .. "m.")
	else
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "You do not have any unjustified frag.")	
	end
	return TRUE
end
