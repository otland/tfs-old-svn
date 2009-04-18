exhaustion =
{
	check = function (cid, storage)
		if(getPlayerStorageValue(cid, storage) >= os.time(t)) then
			return TRUE
		end

		return FALSE
	end,

	get = function (cid, storage)
		local exhaust = getPlayerStorageValue(cid, storage)
		if(exhaust > 0) then
			local left = exhaust - os.time(t)
			if(left >= 0) then
				return left
			end
		end

		return FALSE
	end,

	set = function (cid, storage, time)
		setPlayerStorageValue(cid, storage, os.time(t) + time)
	end,

	make = function (cid, storage, time)
		local exhaust = exhaustion.get(cid, storage)
		if(not exhaust) then
			exhaustion.set(cid, storage, time)
			return TRUE
		end

		return FALSE
	end
}
