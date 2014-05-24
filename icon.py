#!/usr/bin/python

icons = {
    'flag': ['flag'], 
    'bus': ['bus'],
    'airport': ['airport', 'plane'],
    'bicycle': ['bicycle', 'bike'],
    'car': ['car', 'automobile'],
    'caution': ['caution', 'hazard', 'skull'],
    'home': ['home'],
    'info': ['info'],
    'parking': ['parking'],
    'petrol': ['petrol', 'gas', 'fuel'],
    'sail': ['boat', 'sail'],
    'taxi': ['taxi', 'cab'], 
    'academy': ['school', 'university'],
    'computer': ['computer'],
    'beer': ['beer'],
    'restaurant': ['food'],
    'cafe': ['cafe'],
    'bank-dollar': ['money'],
    'shoppingbag': ['shoppingbag'],
    'medical': ['medical'],
    'ski': ['ski']
}

list = {}

print 'icons = {'
for name, values in icons.iteritems():
    list[tuple(values)] = name
    print '    %s: \'%s\',' % (tuple(values), name)
print '}'

