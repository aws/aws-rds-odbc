//  Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Library General Public
//  License as published by the Free Software Foundation; either
//  version 2 of the License, or (at your option) any later version.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Library General Public License for more details.
//
//  You should have received a copy of the GNU Library General Public
//  License along with this library; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

#ifndef HOSTAVAILABILITYSTRATEGY_H_
#define HOSTAVAILABILITYSTRATEGY_H_

typedef enum {
  AVAILABLE,
  NOT_AVAILABLE
} HostAvailability;

class HostAvailabilityStrategy {
public:
    virtual ~HostAvailabilityStrategy() = default;

    virtual void set_host_availability(HostAvailability hostAvailability) = 0;
    virtual HostAvailability get_host_availability(HostAvailability rawHostAvailability) = 0;
};

#endif // HOSTAVAILABILITYSTRATEGY_H_
