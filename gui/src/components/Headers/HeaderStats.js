import React from "react";

// components

import CardStatsBool from "components/Cards/CardStatsBool.js";
import CardStatsValue from "components/Cards/CardStatsValue.js";
import CardControl from "components/Cards/CardControl.js";
import CardSelector from "components/Cards/CardSelector.js";

export default function HeaderStats({ deviceSelected, setDevice }) {

  // Render Header Stats
  return (
    <>
      {/* Header */}
      <div className="relative bg-lightBlue-600 md:pt-32 pb-32 pt-12">
        <div className="px-4 md:px-10 mx-auto w-full">
          <div>
            {/* Card stats */}
            
            <div className="flex flex-wrap">
              <div className="w-full lg:w-6/12 xl:w-3/12 px-4">
                <CardStatsBool
                  statId="light"
                  statTitle="Light Status"
                  statData="1234"
                  statIconName="fas fa-lightbulb"
                  statIconColor="bg-yellow-500"
                  deviceSelected={deviceSelected}
                />
              </div>
              <div className="w-full lg:w-6/12 xl:w-3/12 px-4">
                <CardStatsBool
                  statId="fire"
                  statTitle="Fire"
                  statData="1234"
                  statIconName="fas fa-fire"
                  statIconColor="bg-red-500"
                  deviceSelected={deviceSelected}
                />
              </div>
              <div className="w-full lg:w-6/12 xl:w-3/12 px-4">
                <CardControl
                  statId="light"
                  statTitle="Light Control"
                  statIconName="fas fa-lightbulb"
                  statIconColor="bg-lightBlue-500"
                  deviceSelected={deviceSelected}
                />
              </div>
              <div className="w-full lg:w-6/12 xl:w-3/12 px-4">
                <CardControl
                  statId="air"
                  statTitle="Air Conditioning"
                  statIconName="fas fa-undo"
                  statIconColor="bg-orange-500"
                  deviceSelected={deviceSelected}
                />
              </div>
            </div>

            <div className="space"></div>

            <div className="flex flex-wrap">
              <div className="w-full lg:w-6/12 xl:w-3/12 px-4">
                <CardStatsValue
                  statId="temperature"
                  statTitle="Temperature"
                  statData="1234"
                  statIconName="fas fa-thermometer-empty"
                  statIconColor="bg-lightBlue-500"
                  deviceSelected={deviceSelected}
                />
              </div>
              <div className="w-full lg:w-6/12 xl:w-3/12 px-4">
                <CardStatsValue
                  statId="apparent-temperature"
                  statTitle="Apparent"
                  statData="1234"
                  statIconName="fas fa-thermometer-half"
                  statIconColor="bg-lightBlue-500"
                  deviceSelected={deviceSelected}
                />
              </div>
              <div className="w-full lg:w-6/12 xl:w-3/12 px-4">
                <CardStatsValue
                  statId="humidity"
                  statTitle="Humidity"
                  statData="1234"
                  statIconName="fas fa-tint"
                  statIconColor="bg-lightBlue-500"
                  deviceSelected={deviceSelected}
                />
              </div>
              <div className="w-full lg:w-6/12 xl:w-3/12 px-4">
                <CardSelector
                  statId="device"
                  statTitle="Device"
                  deviceSelected={deviceSelected}
                  onChangeCallback={setDevice}
                />
              </div>
            </div>

          </div>
        </div>
      </div>
    </>
  );
}
