import React from "react";

// components

import CardStats from "components/Cards/CardStats.js";
import CardControl from "components/Cards/CardControl.js";
import CardSelector from "components/Cards/CardSelector.js";

export default function HeaderStats() {
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
                <CardStats
                  statId="light"
                  statTitle="Light Status"
                  statData="1234"
                  statIconName="fas fa-lightbulb"
                  statIconColor="bg-yellow-500"
                />
              </div>
              <div className="w-full lg:w-6/12 xl:w-3/12 px-4">
                <CardStats
                  statId="fire"
                  statTitle="Fire"
                  statData="1234"
                  statIconName="fas fa-fire"
                  statIconColor="bg-red-500"
                />
              </div>
              <div className="w-full lg:w-6/12 xl:w-3/12 px-4">
                <CardControl
                  statId="light"
                  statTitle="Air Conditioning"
                  statIconName="fal fa-solid fa-arrows-rotate"
                  statIconColor="bg-lightBlue-500"
                />
              </div>
              <div className="w-full lg:w-6/12 xl:w-3/12 px-4">
                <CardSelector
                  statId="device"
                  statTitle="Device"
                />
              </div>
            </div>
          </div>
        </div>
      </div>
    </>
  );
}
