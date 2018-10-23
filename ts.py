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

    def swap_zones(self, i, j):
        self.day_to_zone[i], self.day_to_zone[j] = (self.day_to_zone[j], self.day_to_zone[i])
        self.zone_to_day[self.day_to_zone[i]] = i
        self.zone_to_day[self.day_to_zone[j]] = j

    def cost_diff_swap_zones(self, i, j):
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

    def select_city(self, i, j):
        # i, j: zone index, city index
        self.zones_cities[i][0], self.zones_cities[i][j] = (self.zones_cities[i][j], self.zones_cities[i][0])

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