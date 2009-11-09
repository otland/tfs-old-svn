wait = coroutine.yield

function runThread(co)
	if(coroutine.status(co) ~= 'dead') then
		local _, delay = coroutine.resume(co)
		addEvent(continueThread, delay, co)
	end
end

function createThread(data)
	local dataType, func = type(data), nil
	if(dataType == 'string') then
		func = loadstring(data)
	elseif(dataType == 'function') then
		func = data
	end

	if(func ~= nil) then
		local co = coroutine.create(func)
		runThread(co)
	else
		print("[createThread]", "Invalid data specified.")
	end
end
