import React from "react";

// components

import CardProfile from "components/Cards/CardProfile.js";

export default function Settings() {
  return (
    <>
      <div className="flex flex-wrap">
        <div className="w-full lg:w-6/12 px-4">
          <CardProfile 
            imageUrl="https://avatars.githubusercontent.com/u/4183824?v=4"
            name="Gabriele De Rosa"
            university="Università degli Studi di Milano - Bicocca"
            place="Milano"
            githubUsername="derogab"
          />
        </div>
        <div className="w-full lg:w-6/12 px-4">
          <CardProfile 
            imageUrl="https://avatars.githubusercontent.com/u/32036043?v=4"
            name="Federica Di Lauro"
            university="Università degli Studi di Milano - Bicocca"
            place="Milano"
            githubUsername="fdila"
          />
        </div>
      </div>
    </>
  );
}
