import React from "react";

// components

import CardLineChart from "components/Cards/CardLineChart.js";

export default function DataLight({ deviceSelected }) {
  return (
    <>
      <div className="flex flex-wrap">
        <div className="w-full xl:w-12/12 mb-12 xl:mb-0 px-4">
          <CardLineChart deviceSelected={deviceSelected} dataType="light" />
        </div>
      </div>
    </>
  );
}
