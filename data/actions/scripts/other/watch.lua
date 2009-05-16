local config = {
	tibianTime = true,
	twentyFour = true -- only if tibianTime = false
}

function onUse(cid, item, fromPosition, itemEx, toPosition)
	local _time = ""
	if(config.tibianTime) then
		local varh = (os.date('%M') * 60 + os.date('%S')) / 150
		local tibH = math.floor(varh)
		local tibM = math.floor(60 * (varh - tibH))

		if(tibH < 10) then
			tibH = '0' .. tibH
		end
		if(tibM < 10) then
			tibM = '0' .. tibM
		end

		_time = tibH .. ':' .. tibM
	else
		if(config.twentyFour) then
			_time = os.date('%H:%M')
		else
			_time = os.date('%I:%M %p')
		end
	end

	doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "The time is " .. _time .. ".")
	return true
end
