import React, { useState } from "react";
import { BrowserRouter, Route, Switch, Redirect } from "react-router-dom";

import "@fortawesome/fontawesome-free/css/all.min.css";
import "assets/styles/tailwind.css";
import "assets/styles/custom.css";

// components

import Navbar from "components/Navbars/Navbar.js";
import Sidebar from "components/Sidebar/Sidebar.js";
import HeaderStats from "components/Headers/HeaderStats.js";
import Footer from "components/Footers/Footer.js";

// views

import Dashboard from "views/Dashboard.js";
import DataApparentTemperature from "views/DataApparentTemperature";
import DataTemperature from "views/DataTemperature";
import DataHumidity from "views/DataHumidity";
import DataLight from "views/DataLight";
import DataFire from "views/DataFire";
import About from "views/About.js";


export default function App() {

  // Init stateful
  const [deviceSelected, setDevice] = useState(null);

  // Render app
  return (
    <>
      <BrowserRouter>
          <Sidebar />
          <div className="relative md:ml-64 bg-blueGray-100">
              <Navbar />
              {/* Header */}
              <HeaderStats deviceSelected={deviceSelected} setDevice={setDevice} />
              <div className="px-4 md:px-10 mx-auto w-full -m-24">
              <Switch>
                <Route path="/dashboard" exact component={Dashboard} />
                <Route path="/temperature" exact render={(props) => <DataTemperature deviceSelected={deviceSelected} {...props} /> } />
                <Route path="/apparent-temperature" exact render={(props) => <DataApparentTemperature deviceSelected={deviceSelected} {...props} /> } />
                <Route path="/humidity" exact render={(props) => <DataHumidity deviceSelected={deviceSelected} {...props} /> } />
                <Route path="/light" exact render={(props) => <DataLight deviceSelected={deviceSelected} {...props} /> } />
                <Route path="/fire" exact render={(props) => <DataFire deviceSelected={deviceSelected} {...props} /> } />
                <Route path="/about" exact component={About} />
                <Redirect from="/" to="/dashboard" />
              </Switch>
              <Footer />
              </div>
          </div>
      </BrowserRouter>
    </>
  );
}