class Dataset:

    def __init__(self, zones_cities, C, max_price):
        # zones_cities (list of lists): list of zones, where each zone is a list of cities (strings),
        # first zone contains only the starting city, the last zone is the same as the first one but with all cities
        # C (3D numpy array): cost of flights, indices correspond to departure city, arrival city and the day of flight,
        #                     values are normed to [0, 1], 10 indicates no flight
        # max_price (float): maximal price of flight. C * max_price equals true flight prices

        self.zones_cities = self.zones_cities_to_indices(zones_cities)
        self.C = C / max_price
        self.max_price = max_price

        self.path_len = len(self.zones_cities)
        self.day_to_zone = list(range(self.path_len))
        self.zone_to_day = list(range(self.path_len))

        self.select_city_idxs = self.get_select_city_idxs()

    def zones_cities_to_indices(self, zones_cities):

        ns = list(map(len, zones_cities))
        idxs = range(sum(ns))

        zone_city_to_index = []
        n = 0
        for i in range(len(zones_cities)):
            zone_city_to_index.append(list(idxs[n:n + ns[i]]))
            n += ns[i]

        zone_city_to_index = [[0]] + zone_city_to_index[1:] + zone_city_to_index[:1]

        return zone_city_to_index

    def get_select_city_idxs(self):

        idxs = []
        for i in range(self.path_len):
            for j in range(len(self.zones_cities[i])):
                if j > 0:
                    idxs.append((i, j))
        return idxs

    def get_path(self):
        return [self.path(i) for i in range(self.path_len)]

    def path(self, i, j=0):
        # i: day index, j: index of visited city in a zone
        return self.zones_cities[self.day_to_zone[i]][j]

    def path_cost(self):

        cost = 0.
        prev_city = self.path(0)

        for i in range(1, self.path_len):
            act_city = self.path(i)
            cost += self.C[prev_city, act_city, i - 1]
            prev_city = act_city

        return cost

    def swap(self, i, j):
        self.day_to_zone[i], self.day_to_zone[j] = (
            self.day_to_zone[j], self.day_to_zone[i])
        self.zone_to_day[self.day_to_zone[i]] = i
        self.zone_to_day[self.day_to_zone[j]] = j

    def cost_diff_swap(self, i, j):
        # i, j: day indices!

        if abs(i - j) > 1:

            cost_before = (self.C[self.path(i - 1), self.path(i), i - 1] +
                           self.C[self.path(i), self.path(i + 1), i] +
                           self.C[self.path(j - 1), self.path(j), j - 1] +
                           self.C[self.path(j), self.path(j + 1), j])

            cost_after = (self.C[self.path(i - 1), self.path(j), i - 1] +
                          self.C[self.path(j), self.path(i + 1), i] +
                          self.C[self.path(j - 1), self.path(i), j - 1] +
                          self.C[self.path(i), self.path(j + 1), j])
        else:

            k = min(i, j)
            l = max(i, j)

            cost_before = (self.C[self.path(k - 1), self.path(k), k - 1] +
                           self.C[self.path(k), self.path(l), k] +
                           self.C[self.path(l), self.path(l + 1), l])

            cost_after = (self.C[self.path(k - 1), self.path(l), k - 1] +
                          self.C[self.path(l), self.path(k), k] +
                          self.C[self.path(k), self.path(l + 1), l])

        return cost_after - cost_before

    def revert(self, i, j):
        # revert path between i-th and j-th index

        k = min(i, j)
        l = max(i, j)

        m = (l - k + 1) // 2

        for i in range(m):
            self.day_to_zone[k + i], self.day_to_zone[l - i] = \
                self.day_to_zone[l - i], self.day_to_zone[k + i]
            self.zone_to_day[self.day_to_zone[k + i]] = k + i
            self.zone_to_day[self.day_to_zone[l - i]] = l - i

    def cost_diff_revert(self, i, j):

        k = min(i, j)
        l = max(i, j)

        cost_before = (self.C[self.path(k - 1), self.path(k), k - 1] +
                       self.C[self.path(l), self.path(l + 1), l])

        cost_after = (self.C[self.path(k - 1), self.path(l), k - 1] +
                      self.C[self.path(k), self.path(l + 1), l])

        for m in range(l - k):
            cost_before += self.C[self.path(k + m), self.path(k + m + 1), k + m]
            cost_after += self.C[self.path(l - m), self.path(l - m - 1), k + m]

        return cost_after - cost_before

    def insert(self, i, j):
        self.day_to_zone.insert(j, self.day_to_zone.pop(i))
        k = min(i, j)
        l = max(i, j)
        for i in range(k, l + 1):
            self.zone_to_day[self.day_to_zone[i]] = i

    def cost_diff_insert(self, i, j):

        if i < j:
            cost_before = (self.C[self.path(i - 1), self.path(i), i - 1] +
                           self.C[self.path(j - 1), self.path(j), j - 1] +
                           self.C[self.path(j), self.path(j + 1), j])

            cost_after = (self.C[self.path(i - 1), self.path(i + 1), i - 1] +
                          self.C[self.path(j), self.path(i), j - 1] +
                          self.C[self.path(i), self.path(j + 1), j])

            for k in range(i, j - 1):
                cost_before += self.C[self.path(k), self.path(k + 1), k]
                cost_after += self.C[self.path(k + 1), self.path(k + 2), k]

        if j < i:
            cost_before = (self.C[self.path(j - 1), self.path(j), j - 1] +
                           self.C[self.path(j), self.path(j + 1), j] +
                           self.C[self.path(i), self.path(i + 1), i])

            cost_after = (self.C[self.path(j - 1), self.path(i), j - 1] +
                          self.C[self.path(i), self.path(j), j] +
                          self.C[self.path(i - 1), self.path(i + 1), i])

            for k in range(j + 1, i):
                cost_before += self.C[self.path(k), self.path(k + 1), k]
                cost_after += self.C[self.path(k - 1), self.path(k), k]

        return cost_after - cost_before

    def select_city(self, i, j):
        # i, j: zone index, city index
        self.zones_cities[i][0], self.zones_cities[i][j] = (
            self.zones_cities[i][j], self.zones_cities[i][0])

    def cost_diff_select_city(self, i, j):
        # i, j: zone index, city index

        day = self.zone_to_day[i]

        city_before = self.path(day - 1)
        cost_before = self.C[city_before, self.zones_cities[i][0], day - 1]
        cost_after = self.C[city_before, self.zones_cities[i][j], day - 1]

        if day < self.path_len - 1:

            city_after = self.path(day + 1)
            cost_before += self.C[self.zones_cities[i][0], city_after, day]
            cost_after += self.C[self.zones_cities[i][j], city_after, day]

        return cost_after - cost_before






def sa_solve(d, T, t_start):

    cost = d.path_cost()

    min_path = d.get_path()
    min_cost = cost

    costs = []
    cd = []

    n_select_city = 0
    n_swap = 0
    n_insert = 0
    n_revert = 0

    n_cities = sum(map(len, d.zones_cities))

    permutation_idxs = list(range(1, d.path_len - 1))
    selection_idxs = list(range(len(d.select_city_idxs)))

    prob_select_city = len(d.select_city_idxs) / n_cities
    print('zones_cities', list(map(len, d.zones_cities)))
    print('prob_select_city', prob_select_city)
    select_city_rands = random.rand(len(T))

    for idx, t in enumerate(T):

        select_city_cond = select_city_rands[idx] < prob_select_city
        if select_city_cond:

            i, j = d.select_city_idxs[random.choice(selection_idxs)]
            cost_diff = d.cost_diff_select_city(i, j)

        else:

            i, j = random.choice(permutation_idxs, 2, replace=False)
            if abs(i - j) <= 30:
                swap_cost_diff = d.cost_diff_swap(i, j)
                insert_cost_diff = d.cost_diff_insert(i, j)
                revert_cost_diff = d.cost_diff_revert(i, j)
                cost_diffs = [swap_cost_diff, insert_cost_diff, revert_cost_diff]
                method_idx = argmin(cost_diffs)
                cost_diff = cost_diffs[method_idx]
            else:
                cost_diff = d.cost_diff_swap(i, j)
                method_idx = 0

        accept = True

        if cost_diff > 0:
            if random.rand() > exp(-cost_diff / t):
                accept = False

        if accept:

            if select_city_cond:
                d.select_city(i, j)
                n_select_city += 1
            else:
                if method_idx == 0:
                    d.swap(i, j)
                    n_swap += 1
                elif method_idx == 1:
                    d.insert(i, j)
                    n_insert += 1
                elif method_idx == 2:
                    d.revert(i, j)
                    n_revert += 1

            cost += cost_diff

            if cost < min_cost:
                min_cost = cost
                min_path = list(d.get_path())

        if idx % 100 == 0 and idx > 0:

            assert(abs(d.path_cost() - cost) < 1e-6)

            progress = idx / len(T)
            seconds_elapsed = (datetime.now() - t_start).total_seconds()
            estimated_total_time_seconds = seconds_elapsed / progress
            estimated_remaining_seconds = estimated_total_time_seconds - seconds_elapsed

            #print("\r" + 'progress: ' + str(100 * (idx / len(T))) + '%' + ',  estimated remaining time: ' + str(Timedelta(seconds=estimated_remaining_seconds)),)
            print('progress {}%, estimated remaining time {}'.format(
                round(100 * (idx / len(T)), 2), Timedelta(seconds=estimated_remaining_seconds)),
            )#end="\r")

            costs.append(cost)
            cd.append(cost_diff)

    print('n_select_city', n_select_city)
    print('n_swap', n_swap)
    print('n_insert', n_insert)
    print('n_revert', n_revert)

    return costs, cd, min_cost, min_path


