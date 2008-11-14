exhaustion =
{
	check = function (cid, storage)
		local exhaust = getPlayerStorageValue(cid, storage)
		if (os.time(t) >= exhaust) then
			return FALSE
		else
			return TRUE
		end
	end,

	get = function (cid, storage)
		local exhaust = getPlayerStorageValue(cid, storage)
		local left = exhaust - os.time(t)
		if (left >= 0) then
			return left
		else
			return FALSE
		end
	end,

	set = function (cid, storage, time)
		setPlayerStorageValue(cid, storage, os.time(t) + time)
	end,

	make = function (cid, storage, time)
		local exhaust = exhaustion.get(cid, storage)
		if (exhaust > 0) then
			return FALSE
		else
			exhaustion.set(cid, storage, time)
			return TRUE
		end
	end
}
