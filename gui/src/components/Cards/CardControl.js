import React, { useState } from "react";
import PropTypes from "prop-types";

// CardControl
const CardControl = ({
  statId,
  statTitle,
  statIconName,
  statIconColor,
}) => {
  // Init stateful
  const [value, setValue] = useState(false);

  // Print data
  return (
    <>
      <div id={statId} className="relative flex flex-col min-w-0 break-words bg-white rounded mb-6 xl:mb-0 shadow-lg pointer fixed-min-height" onClick={async () => {
        
        // Set status
        const status = (!value) ? 'on' : 'off';
        // Send local request to API
        const resss = await fetch('http://localhost:3001/control/807D3A42D1C5/' + statId + '/' + status, {
          method: 'GET',
          headers: {
            "Access-Control-Allow-Origin": "*",
            "Content-Type": "application/json",
          },
        });
        // Logs to console
        console.log('CardControl / Set: ' + status.toUpperCase());
        console.log('CardControl / Response: ',  resss);
        // Change value
        setValue(!value);
        
      }}>
        <div className="flex-auto p-4">
          <div className="flex flex-wrap">
            <div className="relative w-full pr-4 max-w-full flex-grow flex-1">
              <h5 className="text-blueGray-400 uppercase font-bold text-xs">
                {statTitle}
              </h5>
              <span className="font-semibold text-xl text-blueGray-700">
                {value ? "YES": "NO"}
              </span>
            </div>
            <div className="relative w-auto pl-4 flex-initial">
              <div
                className={
                  "text-white p-3 text-center inline-flex items-center justify-center w-12 h-12 shadow-lg rounded-full " +
                  statIconColor
                }
              >
                <i className={statIconName}></i>
              </div>
            </div>
          </div>
        </div>
      </div>
    </>
  );
};

CardControl.defaultProps = {
  statId: "light",
  statTitle: "Example",
  statIconName: "far fa-chart-bar",
  statIconColor: "bg-red-500",
};

CardControl.propTypes = {
  statId: PropTypes.string,
  statTitle: PropTypes.string,
  // can be any of the text color utilities
  // from tailwindcss
  statIconName: PropTypes.string,
  // can be any of the background color utilities
  // from tailwindcss
  statIconColor: PropTypes.string,
};

export default CardControl;
